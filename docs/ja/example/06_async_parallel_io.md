# 例 6: async/await + spawn 並列ファイル処理

> 大容量ログファイルを非同期に読み込み、
> 並列で解析し、spawn でバックグラウンド作業を処理する。

---

## 概要

- `async` 関数でノンブロッキング I/O
- `await` で非同期結果を収集
- `spawn` でファイア・アンド・フォーゲットのバックグラウンド作業
- `parallel for` + `reduction` で並列集計
- `try/catch` でエラー回復

---

## コード

```dri
module log_analyzer;

# ── 1. ログレコードの構造 ──────────────────────────────────

class LogRecord {
    String timestamp;
    String level;      # ERROR、WARN、INFO、DEBUG
    String message;
    int line_no;
}

# ── 2. 非同期ファイル読み込み ─────────────────────────────

async list<String> read_lines_async(String path) {
    try {
        String content = await io.read_file(path);
        list<String> lines = str.split(content, "\n");
        return lines;
    } catch (e) {
        print(`ファイル読み込み失敗 [${path}]: ${e}`);
        return [];
    }
}

# ── 3. ログのパース ──────────────────────────────────────

# 形式: "2024-01-15 14:23:45 [ERROR] エラーメッセージ"
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

# ── 4. 並列解析 ──────────────────────────────────────────

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

    # 並列カウント — reduction でレースコンディションなく集計
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

    # エラーメッセージのみを抽出 (逐次処理)
    for (i in 0..result.total) {
        if (str.contains(lines[i], "[ERROR]")) {
            lst.push(result.error_messages, lines[i]);
        }
    }

    return result;
}

# ── 5. マルチファイルの並列処理 ──────────────────────────

async AnalysisResult process_file(String path) {
    print(`処理開始: ${path}`);
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
    print(`完了: ${path} — ${result.errors} 件のエラー、${result.warnings} 件の警告`);
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

# ── 6. バックグラウンドモニタリング ──────────────────────

spawn {
    # バックグラウンドでリアルタイムモニタリング (ファイア・アンド・フォーゲット)
    for (tick in 0..5) {
        sys.sleep(1000);    # 1 秒待機
        print(`[モニター] ${tick + 1} 秒経過`);
    }
}

# ── 7. 実行フロー ─────────────────────────────────────────

# 実際のファイルがないためインメモリシミュレーション
list<String> fake_log = [
    "2024-01-15 10:00:01 [INFO] サーバー起動",
    "2024-01-15 10:00:02 [INFO] データベース接続成功",
    "2024-01-15 10:05:33 [WARN] メモリ使用量 80% 超過",
    "2024-01-15 10:07:45 [ERROR] NullPointerException in UserService",
    "2024-01-15 10:07:46 [ERROR] トランザクションロールバック",
    "2024-01-15 10:08:01 [INFO] 再試行中",
    "2024-01-15 10:08:05 [INFO] 復旧完了",
    "2024-01-15 10:15:22 [WARN] 応答時間 > 500ms",
    "2024-01-15 10:20:00 [ERROR] ディスク容量不足",
    "2024-01-15 10:20:01 [INFO] サーバーシャットダウン",
];

AnalysisResult result = analyze_parallel(fake_log);

print("=== ログ解析結果 ===");
print(`総行数: ${result.total}`);
print(`エラー: ${result.errors}`);
print(`警告: ${result.warnings}`);
print(`情報: ${result.info_count}`);
print(`エラー率: ${result.errors * 100 / result.total}%`);

if (lst.length(result.error_messages) > 0) {
    print("\n=== エラーメッセージ一覧 ===");
    for (msg of result.error_messages) {
        print(`  ⚠ ${msg}`);
    }
}

# タイムスタンプの抽出 + 重複除去
list<String> timestamps = fake_log
    .filter(|String line| -> str.contains(line, "[ERROR]"))
    .map(|String line| -> str.left(line, 10));

print("\nエラー発生日:");
for (ts of lst.unique(timestamps)) { print(`  ${ts}`); }
```

---

## 非同期パターンのまとめ

```dri
# 非同期関数の宣言
async ReturnType func_name(params) { ... }

# await: 非同期結果を待つ
var result = await some_async_func();

# spawn: 結果を必要としないバックグラウンド作業
spawn { long_running_task(); }

# 複数の非同期タスクを同時に開始
var f1 = process_file("log1.txt");  # 即座に開始
var f2 = process_file("log2.txt");  # 即座に開始
var r1 = await f1;                  # 結果を収集
var r2 = await f2;                  # 結果を収集
```

---

## 学習ポイント

- `async`/`await` で I/O 待機中の CPU 無駄をなくす
- `spawn` は結果を待たない作業に使用
- `parallel for reduction` でレースコンディションなく並列集計
- 複数の async タスクを同時に開始し、順番に `await`
