# 拡張関数リファレンス
> 基本の組み込み関数に加えて、`math`, `str`, `lst`, `map`, `io`, `sys` ネームスペースの拡張関数と standalone のユーティリティ関数をまとめます。

---

## math.* — 拡張数学関数

| 関数 | 引数 | 戻り値 | 説明 |
|------|------|------|------|
| `math.pow(x, n)` | 2 | double | x の n 乗 |
| `math.log(x)` | 1 | double | 自然対数 ln(x) |
| `math.log2(x)` | 1 | double | 底 2 の対数 |
| `math.log10(x)` | 1 | double | 底 10 の対数 |
| `math.exp(x)` | 1 | double | e^x |
| `math.asin(x)` | 1 | double | 逆サイン（ラジアン） |
| `math.acos(x)` | 1 | double | 逆コサイン（ラジアン） |
| `math.atan(x)` | 1 | double | 逆タンジェント（ラジアン） |
| `math.atan2(y, x)` | 2 | double | 逆タンジェント（4象限区別） |
| `math.hypot(x, y)` | 2 | double | sqrt(x²+y²) |
| `math.clamp(x, lo, hi)` | 3 | double | lo ≤ x ≤ hi の範囲に制限 |
| `math.lerp(a, b, t)` | 3 | double | 線形補間: a+(b-a)\*t |
| `math.pi()` | 0 | double | π ≈ 3.14159... |
| `math.e()` | 0 | double | e ≈ 2.71828... |
| `math.sign(x)` | 1 | int | 符号: -1 / 0 / 1 |
| `math.degrees(x)` | 1 | double | ラジアン → 度 |
| `math.radians(x)` | 1 | double | 度 → ラジアン |
| `math.gcd(a, b)` | 2 | long | 最大公約数 |
| `math.lcm(a, b)` | 2 | long | 最小公倍数 |
| `math.factorial(n)` | 1 | long | n! |
| `math.is_nan(x)` | 1 | boolean | NaN かどうか |
| `math.is_inf(x)` | 1 | boolean | Infinity かどうか |
| `math.floor_div(a, b)` | 2 | long | 床除算 floor(a/b) |
| `math.sinh(x)` | 1 | double | 双曲線サイン |
| `math.cosh(x)` | 1 | double | 双曲線コサイン |
| `math.tanh(x)` | 1 | double | 双曲線タンジェント |

```dri
double area = math.pi() * r * r;
double dist = math.hypot(3.0, 4.0);       # 5.0
double angle = math.atan2(1.0, 1.0);      # π/4
double clamped = math.clamp(x, 0.0, 1.0);
double lerped = math.lerp(0.0, 100.0, 0.5);  # 50.0
double powered = math.pow(2.0, 10.0);     # 1024.0
double logged = math.log(math.e());       # 1.0
int sign = math.sign(-5.0);              # -1

double deg = math.degrees(math.pi());     # 180.0
long g = math.gcd(12, 18);               # 6
long f = math.factorial(5);              # 120
long fd = math.floor_div(-7, 2);         # -4
double sh = math.sinh(1.0);              # ~1.175
```

---

## str.* — 拡張文字列関数

| 関数 | 引数 | 戻り値 | 説明 |
|------|------|------|------|
| `str.join(lst, sep)` | 2 | String | リスト内の文字列を sep で結合 |
| `str.index_of(s, sub)` | 2 | long | 部分文字列の位置（-1: なし） |
| `str.pad_left(s, n)` | 2 | String | 左に空白を埋めて n 文字に揃える |
| `str.pad_right(s, n)` | 2 | String | 右に空白を埋めて n 文字に揃える |
| `str.is_empty(s)` | 1 | boolean | 空文字列かどうか |
| `str.count(s, sub)` | 2 | long | sub の出現回数 |
| `str.reverse(s)` | 1 | String | 文字列の逆順 |
| `str.to_float(s)` | 1 | double | 文字列 → double パース |
| `str.from_float(x)` | 1 | String | double → 文字列変換 |
| `str.from_bool(b)` | 1 | String | `"true"` または `"false"` |
| `str.to_bool(s)` | 1 | boolean | `"true"`/`"1"` → true |
| `str.from_long(n)` | 1 | String | long → 文字列変換 |
| `str.left(s, n)` | 2 | String | 先頭 n 文字 |
| `str.right(s, n)` | 2 | String | 末尾 n 文字 |
| `str.char_code(c)` | 1 | long | ASCII コード |
| `str.from_char_code(n)` | 1 | String | ASCII コード → 文字 |
| `str.upper_first(s)` | 1 | String | 最初の文字だけ大文字 |
| `str.trim_left(s)` | 1 | String | 左の空白のみ除去 |
| `str.trim_right(s)` | 1 | String | 右の空白のみ除去 |
| `str.remove(s, sub)` | 2 | String | sub の出現をすべて除去 |
| `str.truncate(s, n)` | 2 | String | 先頭 n 文字だけ残す |

```dri
list<String> words = [];
lst.push(words, "hello");
lst.push(words, "world");
lst.push(words, "dri");

String joined = str.join(words, ", ");   # "hello, world, dri"
print(joined);

long idx = str.index_of("hello world", "world");   # 6

print(str.pad_left("42", 6));     # "    42"
print(str.pad_right("hi", 5));    # "hi   "
print(str.is_empty(""));          # true
print(str.count("banana", "an")); # 2
print(str.reverse("hello"));      # "olleh"

double pi = str.to_float("3.14159");
print(str.from_float(pi));        # "3.14159"
print(str.from_bool(true));       # "true"
print(str.from_long(9999999999)); # "9999999999"

String s = "Hello, World!";
print(str.left(s, 5));            # "Hello"
print(str.right(s, 6));          # "World!"
print(str.upper_first("hello dri")); # "Hello dri"
print(str.trim_left("   hello"));    # "hello"
print(str.remove("banana", "an"));   # "ba"
print(str.truncate("Hello, World!", 5)); # "Hello"
```

---

## lst.* — 拡張リスト関数

### 変更系関数 (void)

| 関数 | 引数 | 説明 |
|------|------|------|
| `lst.insert(l, i, val)` | 3 | i の位置に val を挿入 |
| `lst.remove(l, i)` | 2 | i 番目の要素を削除 |
| `lst.clear(l)` | 1 | すべての要素を削除 |
| `lst.extend(l, other)` | 2 | other を l の末尾に連結 |

```dri
list<String> fruits = [];
lst.push(fruits, "apple");
lst.push(fruits, "cherry");

lst.insert(fruits, 1, "banana");   # apple, banana, cherry
lst.remove(fruits, 0);             # banana, cherry

list<String> more = [];
lst.push(more, "date");
lst.push(more, "elderberry");
lst.extend(fruits, more);          # banana, cherry, date, elderberry

lst.clear(fruits);
print(lst.size(fruits));           # 0
```

### 照会関数（値を返す）

| 関数 | 引数 | 戻り値 | 説明 |
|------|------|------|------|
| `lst.contains(l, val)` | 2 | boolean | val を含むかどうか |
| `lst.index_of(l, val)` | 2 | long | val のインデックス（-1: なし） |
| `lst.first(l)` | 1 | value | 最初の要素 |
| `lst.last(l)` | 1 | value | 最後の要素 |
| `lst.min(l)` | 1 | String | 最小要素（辞書順） |
| `lst.max(l)` | 1 | String | 最大要素（辞書順） |
| `lst.count(l, val)` | 2 | long | val の出現回数 |
| `lst.is_empty(l)` | 1 | boolean | 空かどうか |

```dri
list<String> nums = [];
lst.push(nums, "10");
lst.push(nums, "20");
lst.push(nums, "30");

print(lst.contains(nums, "20"));    # true
print(lst.index_of(nums, "20"));   # 1
print(lst.first(nums));            # "10"
print(lst.last(nums));             # "30"
print(lst.min(nums));              # "10"
print(lst.max(nums));              # "30"
print(lst.is_empty(nums));         # false
```

### コピー返却関数

| 関数 | 引数 | 戻り値 | 説明 |
|------|------|------|------|
| `lst.reverse(l)` | 1 | list | 逆順コピー |
| `lst.sort(l)` | 1 | list | ソート済みコピー |
| `lst.join(l, sep)` | 2 | String | sep で結合した文字列 |
| `lst.slice(l, start, end)` | 3 | list | `[start..end)` スライス |
| `lst.unique(l)` | 1 | list | 重複を除去（順序維持） |
| `lst.fill(n, val)` | 2 | list | n 個の val で埋めたリスト |
| `lst.copy(l)` | 1 | list | 浅いコピー |

```dri
list<String> data = [];
lst.push(data, "banana");
lst.push(data, "apple");
lst.push(data, "cherry");
lst.push(data, "apple");

var sorted = lst.sort(data);
print(lst.get(sorted, 0));        # "apple"

var rev = lst.reverse(data);
print(lst.get(rev, 0));           # "apple"  (逆順の最初の要素)

String sentence = lst.join(data, " + ");
print(sentence);                   # "banana + apple + cherry + apple"

var part = lst.slice(data, 1, 3);
print(lst.size(part));            # 2

var unique_fruits = lst.unique(data);
print(lst.size(unique_fruits));   # 3

var zeros = lst.fill(5, "0");
print(lst.size(zeros));           # 5
```

---

## map.* — 拡張マップ関数

| 関数 | 引数 | 戻り値 | 説明 |
|------|------|------|------|
| `map.values(m)` | 1 | list | すべての値のリスト |
| `map.clear(m)` | 1 | void | すべての項目を削除 |
| `map.contains_value(m, val)` | 2 | boolean | 値を含むかどうか |
| `map.merge(m1, m2)` | 2 | Map | m2 で m1 を上書きしたコピー |
| `map.copy(m)` | 1 | Map | 浅いコピー |

```dri
Map<String, int> a = {};
map.set(a, "x", 1);
map.set(a, "y", 2);

Map<String, int> b = {};
map.set(b, "y", 99);
map.set(b, "z", 3);

print(map.contains_value(a, 1));    # true

var merged = map.merge(a, b);
# merged = {x:1, y:99, z:3}
print(map.get(merged, "y"));        # 99

var c = map.copy(a);
map.set(c, "w", 5);
print(map.has(a, "w"));             # false (元には影響なし)

map.clear(a);
print(map.size(a));                 # 0
```

---

## io.* — ファイルシステム拡張関数

| 関数 | 引数 | 戻り値 | 説明 |
|------|------|------|------|
| `io.exists(path)` | 1 | boolean | ファイル／ディレクトリの存在確認 |
| `io.delete_file(path)` | 1 | boolean | ファイル削除 |
| `io.file_size(path)` | 1 | long | ファイルサイズ（バイト、-1: エラー） |
| `io.list_dir(path)` | 1 | list\<String\> | ディレクトリ一覧 |
| `io.make_dir(path)` | 1 | boolean | ディレクトリ作成（中間パス含む） |

```dri
if (io.exists("data.txt")) {
    long size = io.file_size("data.txt");
    print("サイズ:", size, "バイト");
} else {
    print("ファイルなし");
};

io.write_file("temp.txt", "一時データ");
io.delete_file("temp.txt");

io.make_dir("output/logs");

list<String> files = io.list_dir(".");
for (f of files) {
    print(f);
};
```

---

## sys.* — システム拡張関数

| 関数 | 引数 | 戻り値 | 説明 |
|------|------|------|------|
| `sys.env(name)` | 1 | String | 環境変数の値（なければ空文字列） |
| `sys.exit(code)` | 1 | void | プログラム終了 |
| `sys.sleep(ms)` | 1 | void | ms ミリ秒待機 |
| `sys.time()` | 0 | long | Unix タイムスタンプ（秒） |

```dri
String home = sys.env("HOME");
print("HOME:", home);

long t0 = sys.time();
# ... 作業 ...
long t1 = sys.time();
print("経過:", t1 - t0, "秒");

sys.sleep(500);    # 0.5 秒待機

int result = compute();
if (result < 0) {
    print("エラー発生");
    sys.exit(1);
};
sys.exit(0);
```

---

## グローバルユーティリティ関数

ネームスペースなしで直接呼び出せる関数です。  
`sort`, `reverse`, `unique` などのリスト操作は `lst.*` ネームスペースを使用してください。

| 関数 | 引数 | 戻り値 | 説明 |
|------|------|------|------|
| `range(n)` | 1 | list | `["0","1",...,"n-1"]` の文字列リスト |
| `range(start, end)` | 2 | list | `[start..end)` の文字列リスト |
| `assert_that(cond, msg)` | 2 | void | 条件が失敗したら例外を throw |

```dri
# range — 整数範囲のリスト生成
var r = range(5);
for (s of r) {
    print(s);    # "0", "1", "2", "3", "4"
};

var r2 = range(3, 8);
for (s of r2) {
    print(s);    # "3", "4", "5", "6", "7"
};

# assert_that — 事前条件チェック
int divide(int a, int b) {
    assert_that(b != 0, "割り算の分母は0にできません");
    return a / b;
};

try {
    int r = divide(10, 0);
} catch (err) {
    print("例外:", err);   # Assertion failed: 割り算の分母は0にできません
};
```

> `sort`, `reverse`, `unique`, `join`, `min`, `max`, `index_of`, `count` などのリスト操作には `lst.*` を使用します。[[lst.* コピー返却関数](#lst-コピー返却関数)] を参照してください。

> 基本の組み込み関数は [[14_builtins.md](14_builtins.md)] を参照してください。

---
