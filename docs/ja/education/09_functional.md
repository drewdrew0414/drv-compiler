# 関数型プログラミング
> ラムダ式、高階関数 (map/filter/reduce)、パイプ演算子でデータ変換パイプラインを構築します。

---

## ラムダ式

```
|パラメータ, ...| -> 式
|パラメータ, ...| { 本体 }
```

```dri
|x| -> x * 2
|x, y| -> x + y
|x| -> x > 0
|| -> 42             # パラメータなし
```

ブロック本体:

```dri
|x| {
    int doubled = x * 2;
    return doubled + 1;
}
```

### キャプチャガイド — `[copy]` / `[ref]`

dri は C++ にトランスパイルされるため、ラムダが外部の変数を参照するときには **どのようにキャプチャするか** を明示する必要があります。
明示しなければコンパイラが寿命を解析して自動決定しますが、意図を明確に伝えるためにキャプチャガイドの利用を推奨します。

```
[copy 変数, ...] |パラメータ| -> 式    # 値コピーキャプチャ
[ref 変数, ...]  |パラメータ| -> 式    # 参照キャプチャ (寿命に注意)
```

```dri
int factor = 10;

# 値コピー — factor が後で変わってもラムダの結果には影響しません
var mul_copy = [copy factor] |int x| -> x * factor;

# 参照キャプチャ — コピーのオーバーヘッドなし、ただし factor の寿命がラムダより長い必要があります
var mul_ref = [ref factor] |int x| -> x * factor;

print(mul_copy(5));   # 50
print(mul_ref(5));    # 50

factor = 99;
print(mul_copy(5));   # 50  — コピーなので変化なし
print(mul_ref(5));    # 495 — 参照なので現在の factor 値を反映
```

| キャプチャ方法 | C++ 変換 | 使用シーン |
|----------|----------|----------|
| `[copy x]` | `[x]` | ラムダが長く生きる場合、または元値の変化が無関係な場合 |
| `[ref x]` | `[&x]` | 性能が重要で寿命が保証される場合 |
| 未指定 | `[=]` または解析結果 | 単純な使い捨てラムダ |

---

## リストの高階関数

### .map() — 各要素を変換

```dri
list<int> nums = [];
lst.push(nums, 1);
lst.push(nums, 2);
lst.push(nums, 3);
lst.push(nums, 4);
lst.push(nums, 5);

var doubled = nums.map(|x| -> x * 2);
# [2, 4, 6, 8, 10]

var squared = nums.map(|x| -> x * x);
# [1, 4, 9, 16, 25]
```

### .filter() — 条件でフィルタリング

```dri
var evens = nums.filter(|x| -> x % 2 == 0);
# [2, 4]

var big = nums.filter(|x| -> x > 3);
# [4, 5]
```

### .reduce() — 累積して畳み込む

```dri
int sum = nums.reduce(0, |acc, x| -> acc + x);
# 15

int product = nums.reduce(1, |acc, x| -> acc * x);
# 120

int max_val = nums.reduce(0, |acc, x| -> max(acc, x));
# 5
```

### .take() — 先頭から N 個

```dri
var first3 = nums.take(3);
# [1, 2, 3]
```

---

## 高階関数のチェイニング

高階関数を連結してデータパイプラインを構築します。

```dri
list<int> nums = [];
lst.push(nums, 1);
lst.push(nums, 2);
lst.push(nums, 3);
lst.push(nums, 4);
lst.push(nums, 5);
lst.push(nums, 6);
lst.push(nums, 7);
lst.push(nums, 8);
lst.push(nums, 9);
lst.push(nums, 10);

# 偶数だけを選び、二乗してから合計
int result = nums
    .filter(|x| -> x % 2 == 0)
    .map(|x| -> x * x)
    .reduce(0, |acc, x| -> acc + x);
# 220 = 4+16+36+64+100
print(result);
```

---

## パイプ演算子 — `|>`

```
式 |> 関数
```

`value |> f` は `f(value)` と同等です。

```dri
int result = 5 |> |x| -> x * 2;    # 10
```

チェイニング:

```dri
double r = 16.0
    |> |x| -> sqrt(x)          # 4.0
    |> |x| -> x * 3.14159;     # 12.566...
print(r);
```

文字列処理パイプライン:

```dri
String processed = "  Hello, World!  "
    |> |s| -> trim(s)
    |> |s| -> toLower(s);
print(processed);   # "hello, world!"
```

### 型推論の限界 — 明示的な型注釈

単一パラメータのパイプラインはコンパイラが型を自動推論します。
**ジェネリクスや高階関数が途中に入ると推論が失敗する**ことがあります。その場合はラムダパラメータに型を明示します。

```dri
# 推論可能: 前の式が int → x は int と決定される
int r = 5 |> |x| -> x * 2;

# 推論不可能な例: list<T>.map で T を確定できない場合
# コンパイルエラー — x の型が不明
var result = some_list |> |x| -> x * 2;

# 解決策: ラムダパラメータに型を明示
var result = some_list |> |int x| -> x * 2;
```

明示型ラムダの構文:

```
|型 パラメータ| -> 式
|型 パラメータ, 型 パラメータ| -> 式
```

```dri
# 複雑なパイプライン — 型を明示
list<double> r = raw_data
    |> |list<int> l| -> l.filter(|int x| -> x > 0)
    |> |list<int> l| -> l.map(|int x| -> x as double * 1.5);
```

> 型注釈がなくて推論に失敗した場合、コンパイラは **明確なエラーメッセージ** を出力します。
> 単純な変換パイプラインはほとんど注釈なしで動作します。

---

## lazy — 遅延評価

`lazy` キーワードを付けると、実際に使用するまで評価を遅延します。

```dri
lazy list large_data = expensive_computation();
# expensive_computation() は large_data を実際に使うときだけ実行されます
```

---

## 実用例

### データ集計

```dri
list<double> temps = [];
lst.push(temps, 22.5);
lst.push(temps, 18.3);
lst.push(temps, 25.7);
lst.push(temps, 20.1);
lst.push(temps, 23.8);

double total = temps.reduce(0.0, |acc, x| -> acc + x);
double avg = total / lst.size(temps) as double;
print("平均:", avg);

double max_t = temps.reduce(temps[0], |acc, x| -> max(acc, x));
print("最高:", max_t);

var hot = temps.filter(|t| -> t > 23.0);
print("23度以上:", lst.size(hot), "日");
```

### 成績処理

```dri
list<int> scores = [];
lst.push(scores, 85);
lst.push(scores, 92);
lst.push(scores, 78);
lst.push(scores, 95);
lst.push(scores, 60);

var passed = scores.filter(|s| -> s >= 70);
int pass_sum = passed.reduce(0, |acc, x| -> acc + x);
double pass_avg = pass_sum as double / lst.size(passed) as double;
print("合格者:", lst.size(passed));
print("合格平均:", pass_avg);
```

### 文字列処理

```dri
list<String> raw = [];
lst.push(raw, "  Alice  ");
lst.push(raw, "BOB");
lst.push(raw, "  charlie");

var clean = raw
    .map(|s| -> trim(s))
    .map(|s| -> toUpper(s))
    .filter(|s| -> length(s) > 3);

for (name of clean) {
    print(name);
};
# ALICE, CHARLIE
```

> list コレクションの基本的な使い方は [[05_collections.md](05_collections.md)] を参照してください。

---
