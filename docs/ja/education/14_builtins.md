# 組み込み関数リファレンス
> dri が標準で提供する組み込み関数と、ネームスペース別の関数一覧です。

---

## ネームスペースのルール

dri の関数は3つの階層に分かれます。

| 階層 | 例 | 判別基準 |
|------|------|----------|
| **グローバル** | `print`, `abs`, `max`, `sqrt`, `length` | ほぼすべてのプログラムに登場する中核的な演算 |
| **ネームスペース** | `math.pow`, `lst.sort`, `str.join`, `io.read_file` | ドメイン別の拡張関数 |
| **tensor 専用グローバル** | `sum`, `dot`, `norm`, `mean` | tensor 型でのみ動作 — 型システムが衝突を防ぎます |

> 判別基準: **「ほぼすべての dri ファイルで使われるか?」** → YES ならグローバル、NO ならネームスペース。

```dri
# グローバル — ネームスペースなしで直接呼び出し
abs(-5)         # 5
max(3, 7)       # 7
sqrt(16.0)      # 4.0
length("hello") # 5

# ネームスペース — 必ず接頭辞が必要
math.pow(2.0, 10.0)     # 1024.0
lst.sort(fruits)         # ソート済みコピー
str.join(words, ", ")    # 文字列結合
io.read_file("a.txt")    # ファイル読み込み
```

---

## 基本 I/O

| 関数 | シグネチャ | 説明 |
|------|----------|------|
| `print` | `print(...)` | 値を出力（空白区切り、改行付き） |
| `input` | `input(varName)` | 標準入力を変数に格納 |

```dri
print("Hello,", "World!");   # Hello, World!
print(42, 3.14, true);

String s;
input(s);
print("入力:", s);
```

> 入出力の基本構文は [[02_basics.md](02_basics.md)] を参照してください。

---

## 数学関数（直接呼び出し）

| 関数 | 引数 | 説明 |
|------|------|------|
| `abs(x)` | 1 | 絶対値 |
| `max(a, b)` | 2 | 最大値 |
| `min(a, b)` | 2 | 最小値 |
| `sqrt(x)` | 1 | 平方根 |
| `floor(x)` | 1 | 切り捨て |
| `ceil(x)` | 1 | 切り上げ |
| `round(x)` | 1 | 四捨五入 |
| `sin(x)` | 1 | サイン（ラジアン） |
| `cos(x)` | 1 | コサイン（ラジアン） |
| `tan(x)` | 1 | タンジェント（ラジアン） |
| `fma(a, b, c)` | 3 | a\*b+c (Fused Multiply-Add) |
| `likely(x)` | 1 | 分岐予測ヒント: 真の可能性が高い |
| `unlikely(x)` | 1 | 分岐予測ヒント: 偽の可能性が高い |

```dri
print(abs(-5));              # 5
print(max(3, 7));            # 7
print(min(3, 7));            # 3
print(sqrt(16.0));           # 4.0
print(floor(3.7));           # 3.0
print(ceil(3.2));            # 4.0
print(round(3.5));           # 4.0
print(sin(3.14159 / 2.0));   # ~1.0
print(fma(3.0, 4.0, 5.0));   # 17.0
```

---

## コレクション共通

| 関数 | 引数 | 説明 |
|------|------|------|
| `length(x)` | 1 | 配列／文字列の長さ |
| `split(s, delim)` | 2 | 文字列分割 → `list<String>` |

```dri
int[] arr = [1, 2, 3];
print(length(arr));         # 3
print(length("hello"));     # 5

list<String> parts = split("a,b,c", ",");
for (p of parts) { print(p); };
# a, b, c
```

---

## 文字列変換組み込み関数

> `toUpper`, `toLower`, `trim`, `replace` は `str.*` ではなく **直接呼び出し** 関数です。

| 関数 | 引数 | 説明 |
|------|------|------|
| `toUpper(s)` | 1 | 大文字変換 |
| `toLower(s)` | 1 | 小文字変換 |
| `trim(s)` | 1 | 前後の空白を除去 |
| `replace(s, old, new)` | 3 | 文字列置換 |

```dri
print(toUpper("hello"));                      # HELLO
print(toLower("WORLD"));                      # world
print(trim("  hi  "));                        # "hi"
print(replace("foo bar", "bar", "baz"));      # foo baz
```

---

## str.* ネームスペース

| 関数 | 引数 | 説明 |
|------|------|------|
| `str.char_at(s, i)` | 2 | i 番目の文字 |
| `str.from_char(c)` | 1 | char → String |
| `str.from_int(n)` | 1 | int → String |
| `str.to_int(s)` | 1 | String → int |
| `str.substr(s, start, len)` | 3 | 部分文字列 |
| `str.starts_with(s, prefix)` | 2 | 接頭辞の確認 |
| `str.ends_with(s, suffix)` | 2 | 接尾辞の確認 |
| `str.contains(s, sub)` | 2 | 含むかどうか |
| `str.repeat(s, n)` | 2 | n 回繰り返し |
| `str.simd_find(s, t)` | 2 | SIMD 部分文字列検索（インデックス、なければ -1） |

```dri
print(str.char_at("hello", 1));               # 'e'
print(str.from_char('A'));                    # "A"
print(str.from_int(42));                      # "42"
print(str.to_int("99"));                      # 99
print(str.substr("hello world", 6, 5));       # "world"
print(str.starts_with("dri lang", "dri"));    # true
print(str.ends_with("file.dri", ".dri"));     # true
print(str.contains("hello world", "world"));  # true
print(str.repeat("ha", 3));                   # "hahaha"
print(str.simd_find("hello world", "world")); # 6
```

---

## lst.* ネームスペース（list 演算）

> `list.*` ではなく **`lst.*`** です。

| 関数 | 引数 | 説明 |
|------|------|------|
| `lst.push(l, val)` | 2 | 末尾に追加 |
| `lst.pop(l)` | 1 | 末尾要素を削除して返す |
| `lst.size(l)` | 1 | サイズ (long) |
| `lst.get(l, i)` | 2 | i 番目の要素 |
| `lst.set(l, i, val)` | 3 | i 番目の要素を変更 |

```dri
list<int> nums = [];
lst.push(nums, 10);
lst.push(nums, 20);
lst.push(nums, 30);
print(lst.size(nums));       # 3
print(lst.get(nums, 1));     # 20
lst.set(nums, 1, 99);
int v = lst.pop(nums);       # 30
print(lst.size(nums));       # 2
```

---

## map.* ネームスペース

| 関数 | 引数 | 説明 |
|------|------|------|
| `map.set(m, key, val)` | 3 | 挿入／更新 |
| `map.get(m, key)` | 2 | 値の取得 |
| `map.get_or(m, key, default)` | 3 | デフォルト値付き取得 |
| `map.has(m, key)` | 2 | キーの存在確認 |
| `map.del(m, key)` | 2 | キーの削除 |
| `map.keys(m)` | 1 | 全キーの一覧 |
| `map.size(m)` | 1 | 項目数 (long) |

```dri
Map<String, int> m = {};
map.set(m, "a", 1);
map.set(m, "b", 2);
print(map.get(m, "a"));          # 1
print(map.get_or(m, "c", 0));   # 0
print(map.has(m, "b"));          # true
map.del(m, "b");
print(map.size(m));              # 1
```

---

## tensor / 統計関数（直接呼び出し）

| 関数 | 引数 | 説明 |
|------|------|------|
| `sum(t)` | 1 | 要素の合計（AVX 自動ディスパッチ） |
| `dot(a, b)` | 2 | 内積 |
| `norm(t)` | 1 | ユークリッドノルム |
| `mean(t)` | 1 | 平均 |
| `std(t)` | 1 | 標準偏差 |
| `median(t)` | 1 | 中央値 |
| `min_index(t)` | 1 | 最小値のインデックス |
| `max_index(t)` | 1 | 最大値のインデックス |
| `avx512_sum(t)` | 1 | AVX-512 直接和 |
| `avx512_dot(a, b)` | 2 | AVX-512 直接内積 |
| `fast_csv_read(path)` | 1 | 高速 CSV パース |

```dri
tensor<5, double> data = {3.0, 1.0, 4.0, 1.0, 5.0};
print(sum(data));          # 14.0
print(mean(data));         # 2.8
print(norm(data));         # ~5.83
print(min_index(data));    # 1
print(max_index(data));    # 4
```

---

## simd.* ネームスペース

| 関数 | 引数 | 説明 |
|------|------|------|
| `simd.fmadd(a, b, c)` | 3 | a\*b+c (FMA 命令) |

```dri
double r = simd.fmadd(3.0, 4.0, 5.0);   # 17.0
```

---

## math.* ネームスペース

| 関数 | 引数 | 説明 |
|------|------|------|
| `math.invsqrt(x)` | 1 | 1/sqrt(x) を高速計算 |

```dri
print(math.invsqrt(4.0));   # 0.5
```

---

## bits.* ネームスペース

| 関数 | 引数 | 説明 |
|------|------|------|
| `bits.popcount(n)` | 1 | ビット 1 の個数 |

```dri
print(bits.popcount(0b10110101));   # 5
```

---

## io.* ネームスペース

| 関数 | 引数 | 説明 |
|------|------|------|
| `io.read_file(path)` | 1 | ファイル読み込み → String |
| `io.write_file(path, content)` | 2 | ファイル書き込み（上書き） → boolean |
| `io.append_file(path, content)` | 2 | ファイル追記 → boolean |
| `io.mmap_read(path)` | 1 | メモリマップトファイル読み込み（コピーなしで直接アクセス） |

```dri
io.write_file("out.txt", "Hello, dri!");
String content = io.read_file("out.txt");
print(content);   # Hello, dri!
io.append_file("out.txt", "\n二行目");
```

> **`io.mmap_read` 使用時の注意**: 返されたマッピング参照を `@region` ブロックの外に脱出させると、  
> **Use-After-Free** 脆弱性が発生します。コンパイラの脱出解析がこれをコンパイルエラーとして遮断します。  
> 詳細は [[10_memory_model.md — @region 脱出解析](10_memory_model.md)] を参照してください。

---

## sys.* ネームスペース

| 関数 | 引数 | 説明 |
|------|------|------|
| `sys.exec(cmd)` | 1 | システムコマンドの実行 → int（終了コード） |
| `sys.affinity(core_id)` | 1 | スレッドを特定の CPU コアに固定 |
| `sys.get_branch_trace()` | 0 | 現在スレッドの分岐履歴を返す → `list<String>` |
| `sys.clear_branch_trace()` | 0 | 現在スレッドの分岐履歴を初期化 |

```dri
int code = sys.exec("echo hello");
print("終了コード:", code);
sys.affinity(0);

# 分岐トレース（@trace アノテーション必須）
list<String> trace = sys.get_branch_trace();
for (entry of trace) {
    print(entry);
};
sys.clear_branch_trace();
```

---

## mem.* ネームスペース

| 関数 | 引数 | 説明 |
|------|------|------|
| `mem.prefetch(ptr)` | 1 | キャッシュプリフェッチ |

```dri
mem.prefetch(large_array[i + 64]);
```

---

## perf.*

| 関数 | 引数 | 説明 |
|------|------|------|
| `perf.now()` | 0 | 現在時刻（ミリ秒、double） |

```dri
double t0 = perf.now();
# 計測対象のコード
double t1 = perf.now();
print("所要:", t1 - t0, "ms");
```

---

## reflect.* ネームスペース

| 関数 | 引数 | 説明 |
|------|------|------|
| `reflect.type_of(x)` | 1 | 型名 → String |
| `reflect.fields(obj)` | 1 | フィールド名の一覧 |
| `reflect.has_field(obj, name)` | 2 | フィールドの存在確認 |
| `reflect.methods(obj)` | 1 | メソッド名の一覧 |

```dri
int x = 42;
print(reflect.type_of(x));   # "int"
```

---

## compile_eval

| 関数 | 引数 | 説明 |
|------|------|------|
| `compile_eval(expr)` | 1 | コンパイル時に式を評価 |

```dri
int BUF = compile_eval(1024 * 1024);   # 1048576
double C = compile_eval(3.14159 / 4.0);
```

---

## wait.*

| 構文 | 説明 |
|------|------|
| `wait.tick(n);` | n ティック待機 |
| `wait.second;` | 1 秒待機 |
| `wait.minute;` | 1 分待機 |
| `wait.hour;` | 1 時間待機 |

```dri
wait.tick(100);
wait.second;
```

> 拡張関数（math, str, lst, map の拡張）は [[15_extensions.md](15_extensions.md)] を参照してください。

---
