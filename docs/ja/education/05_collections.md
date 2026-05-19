# コレクション

---

## array (配列)

### 宣言と初期化

```dri
int[] nums = [1, 2, 3, 4, 5];
double[] prices = [1.99, 2.49, 3.99];
String[] names = ["Alice", "Bob", "Charlie"];
```

### インデックスアクセス

```dri
int[] arr = [10, 20, 30, 40, 50];

print(arr[0]);    # 10
print(arr[4]);    # 50

arr[2] = 99;
print(arr[2]);    # 99
```

### スライス — `arr[start..end]`

`start..end` は start 以上、end **未満** を表します。

```dri
int[] arr = [10, 20, 30, 40, 50];

int[] a = arr[1..3];    # [20, 30]
int[] b = arr[2..4];    # [30, 40]
```

スライス範囲に変数を使う:

```dri
int s = 1;
int e = 4;
int[] slice = arr[s..e];   # [20, 30, 40]
```

### 長さ

```dri
int[] arr = [1, 2, 3, 4, 5];
print(length(arr));   # 5
```

### 配列の走査

```dri
int[] scores = [88, 92, 75, 100, 65];
int total = 0;

for (s of scores) {
    total += s;
};
double avg = total as double / length(scores) as double;
print("平均:", avg);
```

---

## list\<T\> (動的リスト)

> **重要**: list 関連の組み込み関数は `lst.` 名前空間を使用します。(`list.` ではありません。)

### 宣言

```dri
list<int> nums = [];
list<String> words = [];
list<double> values = [];
```

### lst.* 演算

| 関数 | 説明 |
|------|------|
| `lst.push(l, val)` | 末尾に追加 |
| `lst.pop(l)` | 末尾の要素を取り除いて返す |
| `lst.size(l)` | サイズ (long を返す) |
| `lst.get(l, i)` | i 番目の要素 |
| `lst.set(l, i, val)` | i 番目の要素を更新 |

```dri
list<String> lst = [];

lst.push(lst, "apple");
lst.push(lst, "banana");
lst.push(lst, "cherry");

print(lst.size(lst));          # 3
print(lst.get(lst, 0));        # apple
print(lst.get(lst, 1));        # banana

lst.set(lst, 1, "blueberry");
print(lst.get(lst, 1));        # blueberry

String removed = lst.pop(lst); # cherry
print(removed);
print(lst.size(lst));          # 2
```

### list の走査

```dri
list<int> nums = [];
lst.push(nums, 10);
lst.push(nums, 20);
lst.push(nums, 30);

for (n of nums) {
    print(n);
};
```

### list の高階関数

```dri
list<int> nums = [];
lst.push(nums, 1);
lst.push(nums, 2);
lst.push(nums, 3);
lst.push(nums, 4);
lst.push(nums, 5);

var doubled = nums.map(|x| -> x * 2);
var evens   = nums.filter(|x| -> x % 2 == 0);
int total   = nums.reduce(0, |acc, x| -> acc + x);
var first3  = nums.take(3);
```

---

## Map\<K, V\> (ハッシュマップ)

### 宣言

```dri
Map<String, int> scores = {};
Map<String, String> config = {};
Map<int, boolean> flags = {};
```

### map.* 演算

| 関数 | 説明 |
|------|------|
| `map.set(m, key, val)` | 挿入/更新 |
| `map.get(m, key)` | 値の取得 |
| `map.get_or(m, key, default)` | デフォルト付きで値を取得 |
| `map.has(m, key)` | キーの存在確認 |
| `map.del(m, key)` | キーの削除 |
| `map.keys(m)` | 全キーの一覧 (list\<K\>) |
| `map.size(m)` | 要素数 (long) |

```dri
Map<String, int> age = {};

map.set(age, "Alice", 30);
map.set(age, "Bob", 25);
map.set(age, "Charlie", 28);

int a = map.get(age, "Alice");            # 30
int x = map.get_or(age, "Dave", 0);      # 0 (存在しない場合はデフォルト値)
boolean b = map.has(age, "Bob");          # true

map.del(age, "Charlie");
print(map.has(age, "Charlie"));           # false

print(map.size(age));                     # 2
```

### マップリテラル (初期値の設定)

```dri
Map<String, int> m = {"a": 1, "b": 2, "c": 3};
```

### キーの走査

```dri
Map<String, int> stock = {};
map.set(stock, "apple", 100);
map.set(stock, "banana", 50);
map.set(stock, "cherry", 200);

list<String> items = map.keys(stock);
for (item of items) {
    int count = map.get(stock, item);
    print(item, ":", count);
};
```

---

## tensor (数値配列)

### 静的サイズの tensor

```dri
tensor<4, double> v = {1.0, 2.0, 3.0, 4.0};

print(sum(v));      # 10.0
print(norm(v));     # ユークリッドノルム
```

### 動的 tensor (通常の配列に類似)

```dri
tensor data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0];

print(sum(data));
print(mean(data));
print(std(data));
```

### tensor スライスビュー (Zero-Copy View)

多次元 tensor の一部を切り出す際に、**新しいメモリを割り当てずに**元のメモリを指すビュー (View) を返します。
C++ の `std::mdspan` または Strided ポインタへトランスパイルされます。

```
tensor[行範囲, 列範囲]       # 2D スライス
tensor[開始..終了]            # 1D スライス (既存の array 構文と同じ)
```

```dri
# 100×100 行列 (10000 要素)
tensor<2, double> mat = new tensor<2, double>[100, 100];

# コピーなしで 0~9 行、0~9 列を指すビューを作成
var sub = mat[0..10, 0..10];   # メモリ割り当てなし、ビューを返す

print(mean(sub));   # ビュー上で直接演算可能

# ビューは元のデータとメモリを共有
sub[0, 0] = 99.0;
print(mat[0, 0]);   # 99.0 — 元データに反映される
```

1D スライス:

```dri
tensor<8, double> v = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};

var first_half = v[0..4];   # ビュー: 1.0 2.0 3.0 4.0
var second_half = v[4..8];  # ビュー: 5.0 6.0 7.0 8.0

print(dot(first_half, second_half));   # 内積
```

> ビューは `Borrow<tensor>` のセマンティクスに従います — 元データの寿命より長く生存することはできません。

### @layout_soa — Structure of Arrays レイアウト

デフォルトの配列は AoS (Array of Structures) レイアウトです。`@layout_soa` を付けると
SoA (Structure of Arrays) に内部の格納方式が変わり、SIMD のキャッシュ効率を最大化します。

```dri
class Particle {
    public double x;
    public double y;
    public double z;
    public double mass;
};

# AoS: [x0,y0,z0,m0, x1,y1,z1,m1, ...]  — 直感的なアクセス向け
Particle[] particles_aos = new Particle[1000];

# SoA: [x0,x1,...] [y0,y1,...] [z0,z1,...] [m0,m1,...]  — SIMD 最適化
@layout_soa
Particle[] particles_soa = new Particle[1000];

# ソースコードは同じまま — コンパイラが内部レイアウトのみ変換
simd for (i in 0..1000) {
    particles_soa[i].x += particles_soa[i].mass * 0.01;
    # 内部的には: x[i] += mass[i] * 0.01  ← SIMD に最適な連続アクセス
};
```

| レイアウト | メモリ | 用途 |
|----------|--------|--------|
| AoS (デフォルト) | 各オブジェクトのフィールドが連続 | 単一オブジェクトへのアクセスが頻繁な場合 |
| `@layout_soa` | 各フィールドの配列が連続 | SIMD ループ、GPU アップロード |

---

## `split` — 文字列の分割

```dri
list<String> parts = split("a,b,c,d", ",");
for (p of parts) {
    print(p);
};
# a, b, c, d
```

---
