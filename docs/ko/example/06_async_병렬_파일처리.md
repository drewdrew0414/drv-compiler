# 예제 6: async/await + spawn 병렬 파일 처리

> 대용량 로그 파일을 비동기적으로 읽고,
> 병렬로 분석하며, spawn으로 백그라운드 작업을 처리한다.

---

## 개요

- `async` 함수로 논블로킹 I/O
- `await` 로 비동기 결과 수집
- `spawn` 으로 fire-and-forget 백그라운드 작업
- `parallel for` + `reduction` 으로 병렬 집계
- `try/catch` 로 오류 복구

---

## 코드

```drv
module log_analyzer;

# ── 1. 로그 레코드 구조 ──────────────────────────────────────

class LogRecord {
    String timestamp;
    String level;      # ERROR, WARN, INFO, DEBUG
    String message;
    int line_no;
}

# ── 2. 비동기 파일 읽기 ──────────────────────────────────────

async list<String> read_lines_async(String path) {
    try {
        String content = await io.read_file(path);
        list<String> lines = str.split(content, "\n");
        return lines;
    } catch (e) {
        print(`파일 읽기 실패 [${path}]: ${e}`);
        return [];
    }
}

# ── 3. 로그 파싱 ─────────────────────────────────────────────

# 형식: "2024-01-15 14:23:45 [ERROR] 오류 메시지"
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

# ── 4. 병렬 분석 함수 ────────────────────────────────────────

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

    # 총 라인 수
    result.total = lst.length(lines);

    # 병렬 카운팅 — reduction으로 race condition 없이 집계
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

    # 오류 메시지만 추출 (순차 처리)
    for (i in 0..result.total) {
        if (str.contains(lines[i], "[ERROR]")) {
            lst.push(result.error_messages, lines[i]);
        }
    }

    return result;
}

# ── 5. 멀티 파일 병렬 처리 ──────────────────────────────────

async AnalysisResult process_file(String path) {
    print(`처리 시작: ${path}`);
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
    print(`완료: ${path} — ${result.errors}개 오류, ${result.warnings}개 경고`);
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

# ── 6. 백그라운드 모니터링 ──────────────────────────────────

spawn {
    # 백그라운드에서 실시간 모니터링 (fire-and-forget)
    for (tick in 0..5) {
        sys.sleep(1000);    # 1초 대기
        print(`[모니터] ${tick + 1}초 경과`);
    }
}

# ── 7. 실행 흐름 ─────────────────────────────────────────────

# 실제 파일이 없으므로 인메모리 시뮬레이션
list<String> fake_log = [
    "2024-01-15 10:00:01 [INFO] 서버 시작",
    "2024-01-15 10:00:02 [INFO] 데이터베이스 연결 성공",
    "2024-01-15 10:05:33 [WARN] 메모리 사용량 80% 초과",
    "2024-01-15 10:07:45 [ERROR] NullPointerException in UserService",
    "2024-01-15 10:07:46 [ERROR] 트랜잭션 롤백",
    "2024-01-15 10:08:01 [INFO] 재시도 중",
    "2024-01-15 10:08:05 [INFO] 복구 완료",
    "2024-01-15 10:15:22 [WARN] 응답 시간 > 500ms",
    "2024-01-15 10:20:00 [ERROR] 디스크 공간 부족",
    "2024-01-15 10:20:01 [INFO] 서버 종료",
];

AnalysisResult result = analyze_parallel(fake_log);

print("=== 로그 분석 결과 ===");
print(`총 라인: ${result.total}`);
print(`오류: ${result.errors}`);
print(`경고: ${result.warnings}`);
print(`정보: ${result.info_count}`);
print(`오류율: ${result.errors * 100 / result.total}%`);

if (lst.length(result.error_messages) > 0) {
    print("\n=== 오류 메시지 목록 ===");
    for (msg of result.error_messages) {
        print(`  ⚠ ${msg}`);
    }
}

# 타임스탬프 추출 + 중복 제거
list<String> timestamps = fake_log
    .filter(|String line| -> str.contains(line, "[ERROR]"))
    .map(|String line| -> str.left(line, 10));

print("\n오류 발생 날짜:");
for (ts of lst.unique(timestamps)) { print(`  ${ts}`); }
```

---

## 비동기 패턴 요약

```drv
# 비동기 함수 선언
async ReturnType func_name(params) { ... }

# await: 비동기 결과 기다리기
var result = await some_async_func();

# spawn: 결과 필요 없는 백그라운드 작업
spawn { long_running_task(); }

# 여러 비동기 작업 동시 시작
var f1 = process_file("log1.txt");  # 즉시 시작
var f2 = process_file("log2.txt");  # 즉시 시작
var r1 = await f1;                  # 결과 수집
var r2 = await f2;                  # 결과 수집
```

---

## 학습 포인트

- `async`/`await` 로 I/O 대기 중 CPU 낭비 없음
- `spawn` 은 결과를 기다리지 않는 작업에 사용
- `parallel for reduction` 으로 race condition 없는 병렬 집계
- 여러 async 작업을 동시에 시작 후 순서대로 `await`
