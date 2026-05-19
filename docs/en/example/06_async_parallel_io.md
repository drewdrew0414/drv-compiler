# Example 6: async/await + spawn Parallel File Processing

> Asynchronously read large log files, analyse them in parallel,
> and handle background tasks with spawn.

---

## Overview

- `async` functions for non-blocking I/O
- `await` to collect async results
- `spawn` for fire-and-forget background work
- `parallel for` + `reduction` for parallel aggregation
- `try/catch` for error recovery

---

## Code

```dri
module log_analyzer;

# ── 1. Log record structure ───────────────────────────────────

class LogRecord {
    String timestamp;
    String level;      # ERROR, WARN, INFO, DEBUG
    String message;
    int line_no;
}

# ── 2. Async file read ────────────────────────────────────────

async list<String> read_lines_async(String path) {
    try {
        String content = await io.read_file(path);
        list<String> lines = str.split(content, "\n");
        return lines;
    } catch (e) {
        print(`read failed [${path}]: ${e}`);
        return [];
    }
}

# ── 3. Log parsing ────────────────────────────────────────────

# format: "2024-01-15 14:23:45 [ERROR] error message"
Option<LogRecord> parse_log_line(String line, int line_no) {
    if (str.is_empty(str.trim(line))) { return None; }

    list<String> parts = str.split(line, " ");
    if (lst.length(parts) < 4) { return None; }

    LogRecord r;
    r.timestamp = `${parts[0]} ${parts[1]}`;
    r.level     = str.replace(parts[2], "[", "");
    r.level     = str.replace(r.level, "]", "");
    r.message   = str.join(lst.slice(parts, 3, lst.length(parts)), " ");
    r.line_no   = line_no;
    return Some(r);
}

# ── 4. Parallel analysis ──────────────────────────────────────

class AnalysisResult {
    int total;
    int errors;
    int warnings;
    int info_count;
    list<String> error_messages;
}

@bench
AnalysisResult analyze_parallel(list<String> lines) {
    AnalysisResult result;
    result.total = 0;
    result.errors = 0;
    result.warnings = 0;
    result.info_count = 0;
    result.error_messages = [];

    result.total = lst.length(lines);

    # parallel counting — reduction prevents race conditions
    int err_count  = 0;
    int warn_count = 0;
    int info_count = 0;

    parallel for reduction(+:err_count) reduction(+:warn_count) reduction(+:info_count) (i in 0..result.total) {
        String line = lines[i];
        if (str.contains(line, "[ERROR]")) {
            err_count  += 1;
        } else if (str.contains(line, "[WARN]")) {
            warn_count += 1;
        } else if (str.contains(line, "[INFO]")) {
            info_count += 1;
        }
    }

    result.errors    = err_count;
    result.warnings  = warn_count;
    result.info_count = info_count;

    # extract error messages (sequential)
    for (i in 0..result.total) {
        if (str.contains(lines[i], "[ERROR]")) {
            lst.push(result.error_messages, lines[i]);
        }
    }

    return result;
}

# ── 5. Multi-file parallel processing ────────────────────────

async AnalysisResult process_file(String path) {
    print(`processing: ${path}`);
    list<String> lines = await read_lines_async(path);

    if (lst.length(lines) == 0) {
        AnalysisResult empty;
        empty.total = 0;
        empty.errors = 0;
        empty.warnings = 0;
        empty.info_count = 0;
        empty.error_messages = [];
        return empty;
    }

    AnalysisResult result = analyze_parallel(lines);
    print(`done: ${path} — ${result.errors} errors, ${result.warnings} warnings`);
    return result;
}

AnalysisResult merge_results(list<AnalysisResult> results) {
    AnalysisResult merged;
    merged.total = 0;
    merged.errors = 0;
    merged.warnings = 0;
    merged.info_count = 0;
    merged.error_messages = [];

    for (r of results) {
        merged.total      += r.total;
        merged.errors     += r.errors;
        merged.warnings   += r.warnings;
        merged.info_count += r.info_count;
        lst.extend(merged.error_messages, r.error_messages);
    }
    return merged;
}

# ── 6. Background monitoring ──────────────────────────────────

spawn {
    # real-time monitoring in the background (fire-and-forget)
    for (tick in 0..5) {
        sys.sleep(1000);    # wait 1 second
        print(`[monitor] ${tick + 1}s elapsed`);
    }
}

# ── 7. Execution flow ─────────────────────────────────────────

# no actual files — in-memory simulation
list<String> fake_log = [
    "2024-01-15 10:00:01 [INFO] server started",
    "2024-01-15 10:00:02 [INFO] database connected",
    "2024-01-15 10:05:33 [WARN] memory usage over 80%",
    "2024-01-15 10:07:45 [ERROR] NullPointerException in UserService",
    "2024-01-15 10:07:46 [ERROR] transaction rolled back",
    "2024-01-15 10:08:01 [INFO] retrying",
    "2024-01-15 10:08:05 [INFO] recovery complete",
    "2024-01-15 10:15:22 [WARN] response time > 500ms",
    "2024-01-15 10:20:00 [ERROR] disk space low",
    "2024-01-15 10:20:01 [INFO] server shutdown",
];

AnalysisResult result = analyze_parallel(fake_log);

print("=== Log Analysis Results ===");
print(`total lines: ${result.total}`);
print(`errors: ${result.errors}`);
print(`warnings: ${result.warnings}`);
print(`info: ${result.info_count}`);
print(`error rate: ${result.errors * 100 / result.total}%`);

if (lst.length(result.error_messages) > 0) {
    print("\n=== Error Messages ===");
    for (msg of result.error_messages) {
        print(`  ⚠ ${msg}`);
    }
}

# extract timestamps + deduplicate
list<String> timestamps = fake_log
    .filter(|String line| -> str.contains(line, "[ERROR]"))
    .map(|String line| -> str.left(line, 10));

print("\nerror dates:");
for (ts of lst.unique(timestamps)) { print(`  ${ts}`); }
```

---

## Async Pattern Summary

```dri
# declare async function
async ReturnType func_name(params) { ... }

# await: wait for async result
var result = await some_async_func();

# spawn: background work without waiting
spawn { long_running_task(); }

# start multiple async tasks concurrently
var f1 = process_file("log1.txt");  # starts immediately
var f2 = process_file("log2.txt");  # starts immediately
var r1 = await f1;                  # collect result
var r2 = await f2;                  # collect result
```

---

## Key Takeaways

- `async`/`await` eliminates CPU waste while waiting for I/O
- `spawn` is for work where you don't need the result
- `parallel for reduction` aggregates in parallel without race conditions
- Start multiple async tasks simultaneously, then `await` them in order
