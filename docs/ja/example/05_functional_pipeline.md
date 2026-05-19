# 例 5: 関数型データパイプライン

> ラムダ、パイプ演算子、HOF チェーン、モナドパターンを組み合わせた
> 宣言的なデータ変換パイプライン。

---

## 概要

- `|>` パイプ演算子でデータフローの可読性を向上
- `Result<T>` でエラー伝播 (モナドチェーン)
- `filter/map/reduce` HOF チェーン
- ラムダキャプチャ + クロージャパターン
- `Option<T>` による null 安全な処理

---

## コード

```dri
module data_pipeline;

# ── 1. 基本的なパイプライン ─────────────────────────────────

list<int> numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

# 従来のアプローチ (読みにくい)
list<int> evens = numbers.filter(|int x| -> x % 2 == 0);
list<int> squared = evens.map(|int x| -> x * x);
int total = squared.reduce(0, |int acc, int x| -> acc + x);

# パイプ方式 (宣言的、読みやすい)
int result = numbers
    |> numbers.filter(|int x| -> x % 2 == 0)
    |> numbers.map(|int x| -> x * x)
    |> numbers.reduce(0, |int acc, int x| -> acc + x);

print(`偶数の二乗和: ${result}`);  # 4+16+36+64+100 = 220

# ── 2. 複合ラムダ (クロージャ) ──────────────────────────────

double make_multiplier(double factor) {
    # factor をキャプチャするクロージャを返す
    return [copy factor] |double x| -> x * factor;
}

double double_it = make_multiplier(2.0);
double triple_it = make_multiplier(3.0);

double[] vals = [1.0, 2.0, 3.0, 4.0, 5.0];
double[] doubled = vals.map(double_it);
double[] tripled = vals.map(triple_it);

print("2倍:");
for (v of doubled) { print(v); }

# ── 3. ラムダ合成 ─────────────────────────────────────────

# 2 つの関数を合成する高階関数
auto compose(auto f, auto g) {
    return [copy f, copy g] |auto x| -> f(g(x));
}

var add1  = |int x| -> x + 1;
var times2 = |int x| -> x * 2;
var add1_then_double = compose(times2, add1);   # (x+1)*2

list<int> seq = [1, 2, 3, 4, 5];
list<int> composed_result = seq.map(add1_then_double);
for (v of composed_result) { print(v); }   # 4,6,8,10,12

# ── 4. Result<T> モナドパイプライン ──────────────────────

# エラーが発生しうる変換ステップ
Result<int> parse_int(String s) {
    if (str.is_empty(s)) {
        return Err("空文字列");
    }
    # 簡単なパースシミュレーション
    var is_valid = s.filter(|String c| -> c >= "0" and c <= "9");
    if (lst.length(is_valid) != str.length(s)) {
        return Err(`無効な数値: ${s}`);
    }
    return Ok(str.to_int(s));
}

Result<int> validate_positive(int n) {
    if (n <= 0) { return Err(`正の数である必要があります: ${n}`); }
    return Ok(n);
}

Result<double> safe_sqrt(int n) {
    return Ok(math.sqrt(n));
}

# パイプライン: パース → 検証 → 計算
void process_input(String input) {
    Result<int> parsed = parse_int(input);
    match parsed {
        Ok(v) => {
            Result<int> validated = validate_positive(v);
            match validated {
                Ok(n) => {
                    Result<double> result = safe_sqrt(n);
                    match result {
                        Ok(r) => { print(`√${n} = ${r}`); };
                        Err(e) => { print(`平方根エラー: ${e}`); };
                    };
                };
                Err(e) => { print(`検証エラー: ${e}`); };
            };
        };
        Err(e) => { print(`パースエラー: ${e}`); };
    };
}

process_input("16");     # √16 = 4.0
process_input("-5");     # 検証エラー: 正の数である必要があります: -5
process_input("abc");    # パースエラー

# ── 5. Option<T> 安全チェーン ─────────────────────────────

Map<String, String> config = {};
map.set(config, "host", "localhost");
map.set(config, "port", "8080");
# "timeout" は設定されていない

String get_config(String key) {
    if (map.has(config, key)) {
        return map.get(config, key);
    }
    return "";
}

# Option でキーの欠落を処理
Option<String> host = Some(get_config("host"));
Option<String> port = Some(get_config("port"));
Option<String> timeout = None;

match host {
    Some(h) => {
        match port {
            Some(p) => {
                String url = `http://${h}:${p}`;
                match timeout {
                    Some(t) => { print(`URL: ${url}, タイムアウト: ${t}s`); };
                    None    => { print(`URL: ${url}, タイムアウト: デフォルト`); };
                };
            };
            None => { print("ポートなし"); };
        };
    };
    None => { print("ホストなし"); };
};

# ── 6. 統計パイプライン ───────────────────────────────────

double[] data = [2.4, 5.1, 3.8, 9.2, 1.7, 6.3, 4.5, 8.1, 7.6, 3.2];

# 外れ値を除去 (IQR 法)
double q1 = median(lst.take(lst.sort(data), lst.length(data) / 2));
double q3 = median(lst.slice(lst.sort(data), lst.length(data)/2, lst.length(data)));
double iqr = q3 - q1;
double lower = q1 - 1.5 * iqr;
double upper = q3 + 1.5 * iqr;

double[] clean = data.filter(|double x| -> x >= lower and x <= upper);

print(`元のデータ数: ${lst.length(data)}`);
print(`クリーニング後: ${lst.length(clean)}`);
print(`平均: ${mean(clean)}`);
print(`標準偏差: ${std(clean)}`);
print(`中央値: ${median(clean)}`);

# Z スコア正規化
double mu  = mean(clean);
double sig = std(clean);
double[] normalized = clean.map([copy mu, copy sig] |double x| -> (x - mu) / sig);
print("正規化されたデータ:");
for (v of normalized) {
    print(`  ${v}`);
}
```

---

## 関数型パターンのまとめ

| パターン | dri コード | 説明 |
|---------|-----------|------|
| パイプ | `x \|> f` | `f(x)` と同じ |
| マップ | `.map(f)` | 各要素を変換 |
| フィルタ | `.filter(pred)` | 条件を満たす要素を選択 |
| リデュース | `.reduce(init, f)` | 畳み込み演算 |
| 合成 | `compose(f, g)` | `x → g(x) → f(g(x))` |
| クロージャ | `[copy x] \|y\| -> ...` | 外部変数のキャプチャ |

---

## 学習ポイント

- `compose` 関数によるラムダ合成 (`f ∘ g`)
- `Result<T>` チェーンによるエラー伝播 (失敗時に短絡)
- `[copy factor]` と `[ref factor]` のキャプチャセマンティクス
- 統計パイプラインの関数型実装
