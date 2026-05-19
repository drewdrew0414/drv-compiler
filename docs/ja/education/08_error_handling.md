# エラー処理
> エラーを処理する 3 つの方法: `try/catch` (例外ベース)、`Option<T>` (値の不在)、`Result<T>` (成功/失敗 + エラー情報)

---

## try / catch

```
try {
    本体
} catch (変数名) {
    エラー処理
};
```

```dri
try {
    int result = 10 / 0;
    print(result);
} catch (err) {
    print("エラー:", err);
};
```

---

## throw

```
throw 式;
```

`throw` は String メッセージを投げます。

```dri
double divide(double a, double b) {
    if (b == 0.0) {
        throw "0 で割ることはできません";
    };
    return a / b;
};

try {
    double r = divide(10.0, 0.0);
    print(r);
} catch (err) {
    print("例外:", err);
};
# 出力: 例外: 0 で割ることはできません
```

---

## ネストしたエラー処理

```dri
void process_file(String path) {
    if (length(path) == 0) {
        throw "パスが空です";
    };
    String content = io.read_file(path);
    if (length(content) == 0) {
        throw "ファイルが空です";
    };
    print("処理完了:", length(content), "バイト");
};

try {
    process_file("");
} catch (err) {
    print("失敗:", err);   # 失敗: パスが空です
};

try {
    process_file("data.txt");
} catch (err) {
    print("失敗:", err);
};
```

---

## Option\<T\> — 値があるか、ないか

値が存在しない可能性のある検索操作に使用します。`Some(値)` または `None` を返します。

```dri
Option<int> safe_div(int a, int b) {
    if (b == 0) {
        return None;
    };
    return Some(a / b);
};

match safe_div(10, 2) {
    Some(v) => { print("結果:", v); };   # 結果: 5
    None    => { print("失敗"); };
};

match safe_div(10, 0) {
    Some(v) => { print("結果:", v); };
    None    => { print("失敗"); };       # 失敗
};
```

### 検索での Option の活用

```dri
Option<int> find_first_even(int[] arr) {
    for (n of arr) {
        if (n % 2 == 0) {
            return Some(n);
        };
    };
    return None;
};

int[] data = [1, 3, 5, 4, 7];
match find_first_even(data) {
    Some(v) => { print("最初の偶数:", v); };   # 最初の偶数: 4
    None    => { print("偶数なし"); };
};
```

---

## Result\<T\> — 成功または失敗 (エラー情報を含む)

失敗理由を伝える必要がある変換・検証操作に使用します。`Ok(値)` または `Err(メッセージ)` を返します。

```dri
Result<int> parse_int(String s) {
    if (length(s) == 0) {
        return Err("空文字列");
    };
    return Ok(str.to_int(s));
};

match parse_int("42") {
    Ok(v)  => { print("成功:", v); };    # 成功: 42
    Err(e) => { print("エラー:", e); };
};

match parse_int("") {
    Ok(v)  => { print("成功:", v); };
    Err(e) => { print("エラー:", e); };   # エラー: 空文字列
};
```

### Result の連鎖処理

```dri
Result<double> safe_sqrt(double x) {
    if (x < 0.0) {
        return Err("負の数の平方根は実数ではありません");
    };
    return Ok(sqrt(x));
};

Result<double> compute(String input) {
    match parse_int(input) {
        Ok(n) => {
            match safe_sqrt(n as double) {
                Ok(v)  => { return Ok(v); };
                Err(e) => { return Err(e); };
            };
        };
        Err(e) => { return Err("パースエラー: " + e); };
    };
};

match compute("25") {
    Ok(v)  => { print("結果:", v); };   # 結果: 5.0
    Err(e) => { print("エラー:", e); };
};

match compute("-4") {
    Ok(v)  => { print("結果:", v); };
    Err(e) => { print("エラー:", e); };   # エラー: 負の数の平方根は実数ではありません
};
```

---

## try + Option/Result の組み合わせ

```dri
void run_job(String filename) {
    try {
        String content = io.read_file(filename);
        match parse_int(content) {
            Ok(n)  => { print("ファイルの値:", n * 2); };
            Err(e) => { throw "パース失敗: " + e; };
        };
    } catch (err) {
        print("ジョブ失敗:", err);
    };
};

run_job("number.txt");
run_job("missing.txt");
```

---

## エラー処理戦略の比較

| 方式 | 使用状況 |
|------|----------|
| `try/catch` | 外部リソース (ファイル、ネットワーク) へのアクセス |
| `Option<T>` | 値が存在しない可能性のある検索操作 |
| `Result<T>` | 失敗理由を伝える必要がある変換・検証 |

> `match` パターンマッチング構文は [[04_control_flow.md](04_control_flow.md)] を参照してください。

---
