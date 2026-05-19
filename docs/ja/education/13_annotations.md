# アノテーション
> アノテーションは `@` 接頭辞でコンパイラに最適化・レイアウト・エフェクトのヒントを提供します。

---

## 全体一覧

| アノテーション | 適用対象 | 説明 |
|------------|----------|------|
| `@inline` | 関数 | インライン強制 |
| `@pure` | 関数 | 純粋関数（副作用なし） |
| `@fastcall` | 関数 | 高速呼び出し規約 |
| `@abstract` | メソッド | 子クラスの実装を強制 |
| `@stack` | 変数 | スタック強制割り当て |
| `@heap` | 変数 | ヒープ明示割り当て |
| `@local` | 変数 | スタック／アリーナ強制 |
| `@align(N)` | 変数・クラス | Nバイト整列強制（SIMD/NUMA 最適化） |
| `@packed` | クラス | パディングなしの密着レイアウト（継承不可） |
| `@layout_soa` | 配列変数 | Structure of Arrays レイアウト強制 |
| `@noalias` | 関数 | ポインタエイリアシングなし（`restrict`） |
| `@io` | 関数 | I/O エフェクトの明示 |
| `@alloc` | 関数 | メモリ割り当てエフェクトの明示 |
| `@threadsafe` | 関数 | スレッドセーフの明示 |
| `@bench` | 関数 | 実行時間を自動計測 |
| `@specialize` | 関数 | 型ごとの SIMD 特殊化 |
| `@trace` | 関数 | 分岐トレースのインストルメンテーションを自動注入 |
| `@warrant` | 関数・ルール | トゥールミン論証の Warrant 関係を明示 |
| `@rebuttal` | 関数・ルール | トゥールミン論証の Rebuttal 関係を明示 |
| `@defeats` | 関数・ルール | トゥールミン論証の Defeat 関係を明示 |

---

## 関数最適化

### @inline — インライン強制

関数呼び出しのオーバーヘッドを除去します。

```dri
@inline
int square(int x) {
    return x * x;
};

@inline
double clamp(double val, double lo, double hi) {
    if (val < lo) { return lo; };
    if (val > hi) { return hi; };
    return val;
};
```

### @pure — 純粋関数

副作用がなく、同じ入力に対して常に同じ出力を返します。コンパイラは結果をキャッシュしたり、呼び出しを除去したりできます。

```dri
@pure
double normalize(double x, double min, double max) {
    return (x - min) / (max - min);
};

@pure
int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    };
    return a;
};
```

### @fastcall — 高速呼び出し規約

ホットパス関数に適用します。

```dri
@fastcall
void process(int x, int y, int z) {
    print(x + y + z);
};

@fastcall
double lerp(double a, double b, double t) {
    return a + (b - a) * t;
};
```

---

## メモリレイアウト

### @stack — スタック割り当て強制

```dri
void compute() {
    @stack
    double[] local_buf = [0.0, 0.0, 0.0, 0.0];

    simd for (i in 0..4) {
        local_buf[i] = i as double * 2.0;
    };
};
```

### @heap — ヒープ明示割り当て

```dri
@heap
double[] large = new double[10000000];
```

### @packed — パディングなしの構造体

フィールド間のパディングを除去してメモリを節約します。

```dri
@packed
class Pixel {
    public char r;
    public char g;
    public char b;
    public char a;    # ちょうど4バイト
};

@packed
class Vertex {
    public float x;
    public float y;
    public float z;   # ちょうど12バイト
};
```

> **制約**: `@packed` は継承を持たない **リーフデータクラス** にのみ使用できます。  
> 継承クラス（`extends`）に適用したり、`@packed` クラスを継承したりすると **コンパイルエラー** になります。  
> 理由: 継承時に挿入される vtable ポインタの整列が `@packed` によって崩れ、  
> ミスアライメントアクセス（性能低下またはクラッシュ）が発生します。

### @align(N) — SIMD/NUMA メモリ整列

SIMD ベクトル命令は、データが特定のバイト境界に整列されているときに最高性能を発揮します。

```dri
# AVX-512 最適化（64バイト整列）
@align(64)
double[] simd_buf = new double[1000000];

# AVX2 最適化（32バイト整列）
@align(32)
class Vector3D {
    public double x;
    public double y;
    public double z;
};
```

C++ 変換: `alignas(N)` または `__attribute__((aligned(N)))`

| 整列 | 対象 SIMD |
|------|----------|
| `@align(16)` | SSE2 |
| `@align(32)` | AVX2 |
| `@align(64)` | AVX-512 |

### @layout_soa — Structure of Arrays レイアウト

SIMD ループや GPU アップロードに最適な SoA レイアウトで配列を格納します。  
ソースコードは AoS のように書きますが、コンパイラは内部レイアウトのみを変換します。

```dri
class Particle {
    public double x;
    public double y;
    public double z;
};

# 内部: double[] x, double[] y, double[] z の3配列に分割して保存
@layout_soa
Particle[] pts = new Particle[1000];

# ソースコードは同じですが、内部的には x[i] += ... の形で SIMD 最適化されます
simd for (i in 0..1000) {
    pts[i].x += pts[i].mass * 0.01;
};
```

### @local — スタック／アリーナ強制

```dri
@local
int[] buf = new int[64];
```

---

## ベクトル化

### @noalias — ポインタエイリアシングなし

異なるポインタが同じメモリを指さないことを宣言します。`simd for` と併用すると、より積極的なベクトル化が可能になります。

```dri
@noalias
void vec_add(double[] a, double[] b, double[] c, int n) {
    simd for (i in 0..n) {
        c[i] = a[i] + b[i];
    };
};

@noalias
void scale(double[] v, double factor, int n) {
    simd for (i in 0..n) {
        v[i] = v[i] * factor;
    };
};
```

---

## エフェクトシステム

関数がどのような副作用を持つかを明示します。

### @io — I/O を実行

```dri
@io
String read_config(String path) {
    return io.read_file(path);
};

@io
void log(String msg) {
    io.append_file("app.log", msg + "\n");
    print(msg);
};
```

### @alloc — メモリ割り当て

```dri
@alloc
double[] create_buf(int n) {
    return new double[n];
};

@alloc
Map<String, int> build_index(list<String> keys) {
    Map<String, int> idx = {};
    for (i in 0..lst.size(keys)) {
        map.set(idx, lst.get(keys, i), i);
    };
    return idx;
};
```

### @threadsafe — スレッドセーフ

```dri
int counter = 0;

@threadsafe
void safe_increment() {
    counter += 1;
};

@threadsafe
int safe_get() {
    return counter;
};
```

---

## 抽象メソッド

### @abstract — 実装強制

抽象クラス内で、子クラスが必ず実装しなければならないメソッドを示します。

```dri
abstract class Codec {
    @abstract String encode(String input);
    @abstract String decode(String encoded);

    void verify(String original) {
        String enc = this.encode(original);
        String dec = this.decode(enc);
        if (dec == original) {
            print("検証通過");
        } else {
            print("検証失敗");
        };
    };
};

class Base64Codec extends Codec {
    override String encode(String input) {
        return input;
    };

    override String decode(String encoded) {
        return encoded;
    };
};
```

---

## アノテーションの組み合わせ

複数のアノテーションを同時に使うことができます。

```dri
@inline
@pure
@fastcall
double fast_lerp(double a, double b, double t) {
    return a + (b - a) * t;
};

@noalias
@io
void stream_write(double[] data, int n, String path) {
    for (i in 0..n) {
        io.append_file(path, str.from_int(data[i] as int) + "\n");
    };
};

@alloc
@pure
int[] make_range(int n) {
    int[] arr = new int[n];
    for (i in 0..n) {
        arr[i] = i;
    };
    return arr;
};
```

> アノテーション付きの関数宣言例は [[06_functions_and_classes.md](06_functions_and_classes.md)] を参照してください。

---

## 分岐トレース — `@trace`

関数内のすべての `if/else`、`switch/match` 進入点に **インストルメンテーションコードを自動注入** します。  
各分岐の進入時に `(関数名, 条件文字列, 行番号)` をスレッドローカルマップに記録します。  
`sys.get_branch_trace()` で現在スレッドの分岐履歴を取得できます。

```dri
@trace
Result<double> safe_divide(double a, double b) {
    if (b == 0.0) {
        return Err("分母が0");
    };
    return Ok(a / b);
};

safe_divide(10.0, 0.0);

# 分岐履歴の取得
list<String> trace = sys.get_branch_trace();
for (entry of trace) {
    print(entry);
};
# 出力: "safe_divide | b == 0.0 → true | line 3"
```

C++ 変換時には各分岐の前に `__dri_trace_push(__func__, "b == 0.0", __LINE__)` の形で注入されます。  
`dri-toulmin` ライブラリはこの履歴を論証根拠ツリーの構築に活用します。

---

## トゥールミン論証アノテーション — `@warrant`, `@rebuttal`, `@defeats`

`dri-toulmin` ライブラリと連携し、関数・ルール間の **論証関係グラフ** をバイナリにメタデータとして含めます。  
ランタイムのルールエンジンはリフレクションでこのメタデータを読み取り、信頼度スコアを計算します。

> 参照: [park-jun-woo/toulmin](https://github.com/park-jun-woo/toulmin)

| アノテーション | トゥールミン要素 | 意味 |
|-----------|----------|------|
| `@warrant` | Warrant | 「この関数が真ならば主張を支持する」 |
| `@rebuttal` | Rebuttal | 「この関数が真ならば主張に例外を提起する」 |
| `@defeats` | Defeat | 「この関数が真ならば他のルールを無効化する」 |

```dri
# 基本主張 (Claim)
boolean is_safe_speed(double speed_kmh) {
    return speed_kmh <= 100.0;
};

# Warrant: 速度超過は危険という主張を支持
@warrant("is_safe_speed")
boolean no_speeding(double speed_kmh) {
    return speed_kmh < 120.0;
};

# Rebuttal: 高速道路では例外
@rebuttal("is_safe_speed")
boolean highway_exception(String road_type) {
    return road_type == "highway";
};

# Defeats: 緊急車両なら no_speeding ルールを無効化
@defeats("no_speeding")
boolean emergency_vehicle(boolean is_emergency) {
    return is_emergency;
};
```

コンパイラはこのアノテーションを解析し、バイナリに次のメタデータを含めます:

```json
{
  "rules": [
    { "name": "no_speeding",       "type": "warrant",  "target": "is_safe_speed" },
    { "name": "highway_exception", "type": "rebuttal", "target": "is_safe_speed" },
    { "name": "emergency_vehicle", "type": "defeats",  "target": "no_speeding" }
  ]
}
```

> `dri-toulmin` ライブラリ連携と信頼度計算アルゴリズムは [[17_external_libraries.md](17_external_libraries.md)] を参照してください。

---

## スコープアノテーション（Scope Annotations）

一部のアノテーションは単一の関数や変数を超えて **ブロック全体** に適用されます。コンパイラはブロック進入時に該当する pragma を注入し、ブロック終了時に自動で元に戻します。

| アノテーション | C++ 変換 | 説明 |
|------------|---------|------|
| `@fast_math` | `#pragma GCC optimize("fast-math")` | 浮動小数点の結合則を許可、最大ベクトル化 |
| `@strict_math` | `#pragma STDC FENV_ACCESS ON` | IEEE 754 厳格遵守、例外検出 |
| `@noalias` | パラメータに `__restrict__` | 関数内のすべてのポインタ／参照パラメータにエイリアシングなしのヒント |

```dri
@fast_math
double[] dot_product_batch(double[] a, double[] b) {
    # この関数内では fast-math 最適化が有効化されます
    double[] result = [];
    for (i in 0..length(a)) {
        lst.push(result, a[i] * b[i]);
    };
    return result;
};
# 関数終了時に #pragma GCC reset_options が自動注入されます
```

```dri
@noalias
void matrix_multiply(double[] dst, double[] a, double[] b, int n) {
    # dst, a, b は互いに重ならないと宣言 → コンパイラのベクトル化が改善
    parallel simd for (i in 0..n) {
        dst[i] = a[i] * b[i];
    };
};
```

> **静的解析器**: `@noalias` 宣言後、同じ変数を2つのパラメータに渡すとコンパイルエラーが発生します。

```dri
double[] v = [1.0, 2.0, 3.0];
matrix_multiply(v, v, v, 3);  # エラー: @noalias 違反 — v が dst, a, b すべてに渡されている
```

---

## sys.sync — メモリバリア

```dri
use sys;

int shared_counter = 0;

spawn {
    sys.sync.atomic_store(shared_counter, 42);
    sys.sync.fence_release();   # → std::atomic_thread_fence(memory_order_release)
};

sys.sync.fence_acquire();       # → std::atomic_thread_fence(memory_order_acquire)
int val = sys.sync.atomic_load(shared_counter);
```

| 関数 | C++ 変換 | 説明 |
|------|---------|------|
| `sys.sync.fence_full()` | `atomic_thread_fence(seq_cst)` | 完全な逐次一貫性バリア |
| `sys.sync.fence_acquire()` | `atomic_thread_fence(acquire)` | 読み取りバリア |
| `sys.sync.fence_release()` | `atomic_thread_fence(release)` | 書き込みバリア |
| `sys.sync.atomic_load(x)` | `x.load(acquire)` | アトミック読み取り |
| `sys.sync.atomic_store(x, v)` | `x.store(v, release)` | アトミック書き込み |
| `sys.sync.atomic_cas(x, e, n)` | `x.compare_exchange_strong(e, n)` | CAS 演算 |
