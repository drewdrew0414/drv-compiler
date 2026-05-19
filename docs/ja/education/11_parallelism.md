# 並列処理と非同期
> `parallel for` でコレクションを並列処理し、`simd for` で数値ループをベクトル化し、`async/await` と `spawn` で非同期処理を扱います。

---

## ループ接頭辞体系

dri は `for` の前に接頭辞を付けて実行方式を宣言します。

```
for (...)             { }   # 通常の逐次実行
parallel for (...)    { }   # ワークスティーリング並列実行
simd for (...)        { }   # CPU SIMD ベクトル化
parallel simd for (...) { } # 並列 + ベクトル化を同時適用
```

## 並列処理の選択ガイド

| 状況 | 方法 |
|------|------|
| コレクション要素を独立に処理 | `parallel for (x of ...)` |
| 数値計算集約ループ | `simd for (i in 0..N)` |
| 数値計算 + 並列を同時に | `parallel simd for (i in 0..N)` |
| バックグラウンド I/O | `async`/`await` |
| 結果が不要なバックグラウンドタスク | `spawn { }` |

---

## parallel for — 並列反復

```
parallel for (変数 of コレクション) { 本体 };
parallel for (変数 in 開始..終了) { 本体 };
```

ワークスティーリングスレッドプールをベースに、各反復を並列実行します。

```dri
int[] data = [1, 2, 3, 4, 5, 6, 7, 8];

parallel for (item of data) {
    print("処理:", item);
};
```

大規模な並列処理:

```dri
list<String> files = [];
lst.push(files, "a.txt");
lst.push(files, "b.txt");
lst.push(files, "c.txt");
lst.push(files, "d.txt");

parallel for (file of files) {
    String content = io.read_file(file);
    print("読み込み:", file, "サイズ:", length(content));
};
```

---

## simd for — SIMD ベクトル化

```
simd for (変数 in 開始..終了) { 本体 };
simd for (変数 of コレクション) { 本体 };
```

CPU の AVX2/AVX-512 (x86) または NEON (ARM) を自動的に活用します。

```dri
int N = 1024;
double[] a = new double[N];
double[] b = new double[N];
double[] c = new double[N];

simd for (i in 0..N) {
    c[i] = a[i] + b[i];
};
```

`@noalias` と併用すると、より積極的なベクトル化が可能です:

```dri
@noalias
void saxpy(double alpha, double[] x, double[] y, int n) {
    simd for (i in 0..n) {
        y[i] = alpha * x[i] + y[i];
    };
};
```

---

## parallel for reduction — 並列リダクション

並列ループ内で複数のスレッドが一つの変数に累積演算を行うと、**データレース (Race Condition)** が発生します。
`reduction` キーワードは各スレッド別のローカル変数で演算した後、ループ終了時に **アトミックにマージ** します。

```
parallel for (変数 of コレクション) reduction(演算子:変数) { 本体 };
```

| 演算子 | 初期値 | 説明 |
|--------|--------|------|
| `+` | `0` | 合算 |
| `*` | `1` | 乗算 |
| `max` | 最小値 | 最大値 |
| `min` | 最大値 | 最小値 |
| `and` | `true` | 論理 AND |
| `or` | `false` | 論理 OR |

```dri
int[] big_array = new int[1000000];
for (i in 0..1000000) { big_array[i] = i; };

mut int total = 0;
parallel for (item of big_array) reduction(+:total) {
    total += item;
};
print("合計:", total);

# 最大値リダクション
mut int max_val = 0;
parallel for (item of big_array) reduction(max:max_val) {
    if (item > max_val) { max_val = item; };
};
print("最大値:", max_val);
```

複数のリダクションを同時に使う:

```dri
mut double sum = 0.0;
mut double sum_sq = 0.0;

parallel for (x of data) reduction(+:sum) reduction(+:sum_sq) {
    sum += x;
    sum_sq += x * x;
};

double mean = sum / lst.size(data) as double;
double variance = sum_sq / lst.size(data) as double - mean * mean;
```

> C++ OpenMP の `reduction(+:sum)` パターンと同じ意味論です。

---

## parallel simd for — 並列 + ベクトル化の組み合わせ

並列実行と SIMD ベクトル化を同時に適用します。大規模数値計算の中核パターンです。

```dri
int N = 1048576;   # 1M 要素
double[] a = new double[N];
double[] b = new double[N];
double[] result = new double[N];

@noalias
parallel simd for (i in 0..N) {
    result[i] = a[i] * 2.0 + b[i];
};
```

スレッド毎のチャンクサイズを SIMD 幅に合わせて自動整列します。

---

## async / await

`async` 関数内で `await` により I/O をノンブロッキング処理します。

```dri
async void fetch(String path) {
    String data = await io.read_file(path);
    print("データ:", length(data), "バイト");
};

fetch("data.txt");
```

複数の非同期タスクを順次実行:

```dri
async void load_all() {
    String config = await io.read_file("config.json");
    String data   = await io.read_file("data.csv");
    print("設定:", length(config), "バイト");
    print("データ:", length(data), "バイト");
};

load_all();
```

async + try/catch:

```dri
async void safe_fetch(String path) {
    try {
        String content = await io.read_file(path);
        print("成功:", length(content), "バイト");
    } catch (err) {
        print("読み込み失敗:", err);
    };
};

safe_fetch("existing.txt");
safe_fetch("missing.txt");
```

---

## spawn — ファイアアンドフォーゲット

```
spawn { 本体 };
```

結果を待たずにバックグラウンドで実行します。

```dri
spawn {
    print("バックグラウンド開始");
    for (i in 0..1000000) {
        # 重い計算
    };
    print("バックグラウンド完了");
};

print("メイン処理を継続");
```

複数の spawn を同時に開始:

```dri
spawn {
    String log = io.read_file("events.log");
    process_log(log);
};

spawn {
    String cfg = io.read_file("config.json");
    reload_config(cfg);
};

spawn {
    sys.exec("backup.sh");
};
```

---

## 性能計測

```dri
double t0 = perf.now();

parallel for (item of large_list) {
    heavy_work(item);
};

double t1 = perf.now();
print("所要:", t1 - t0, "ms");
```

---

## CPU アフィニティ

特定のコアにスレッドを固定して NUMA/キャッシュ効率を高めます。

```dri
sys.affinity(0);    # コア 0 に固定

simd for (i in 0..1000000) {
    result[i] = compute(i);
};
```

---

## 全体例 — 並列行列乗算

```dri
int N = 256;
double[] A = new double[N * N];
double[] B = new double[N * N];
double[] C = new double[N * N];

# 初期化
for (i in 0..N) {
    for (j in 0..N) {
        A[i * N + j] = i as double + j as double;
        B[i * N + j] = i as double - j as double;
    };
};

double t0 = perf.now();

# 行単位の parallel for
int[] rows = new int[N];
for (i in 0..N) {
    rows[i] = i;
};

parallel for (row of rows) {
    int i = row;
    for (j in 0..N) {
        double s = 0.0;
        simd for (k in 0..N) {
            s = simd.fmadd(A[i * N + k], B[k * N + j], s);
        };
        C[i * N + j] = s;
    };
};

double t1 = perf.now();
print("行列乗算完了:", t1 - t0, "ms");
```

---

## スレッド安全性アナライザ

dri コンパイラはビルド時に **静的スレッド安全性解析** を自動実行します。

### 検出するパターン

**1. 同期なしの共有変数への書き込み**

```dri
int total = 0;

parallel for (x of data) {
    total += x;   # 警告: total に atomic/reduction なしで書き込み
};
```

コンパイラ警告:
```
warning: possible thread-unsafe write to 'total' inside parallel for
         — add reduction(+:total) or atomic<int>, or use sys.sync.fence_full()
```

**解決策 1: reduction**
```dri
int total = 0;
parallel for reduction(+:total) (x of data) {
    total += x;   # OK — OpenMP reduction に変換される
};
```

**解決策 2: atomic**
```dri
atomic<int> total = 0;
parallel for (x of data) {
    sys.sync.atomic_store(total, sys.sync.atomic_load(total) + x);
};
```

### アナライザの無効化

```bash
dri main.dri --exe app --no-analyze   # 静的解析をスキップ (リリースビルドの高速化)
```

---

## ハードウェアトポロジ認識スケジューラ (sys.topology)

現代の CPU は同じ L3 キャッシュを共有するコアが、ダイ (Die) または CCD (Core Complex Die) 単位でまとめられています。`sys.topology.*` を使うとこの構造を認識し、スレッドを最適な位置に配置できます。

### Intel P-core / E-core 認識

```dri
use sys;

list<int> fast_cores = sys.topology.p_cores();  # 高性能コア ID のリスト
list<int> eff_cores  = sys.topology.e_cores();  # 効率コア ID のリスト
int total = sys.topology.total();
String vendor = sys.topology.vendor();  # "Intel" / "AMD" / "other"

print("論理コア総数:", total);
print("P-コア:", fast_cores);
print("E-コア:", eff_cores);
```

### AMD CCD (L3 キャッシュ共有グループ)

```dri
use sys;

# L3 キャッシュを共有するコアグループのリスト (AMD CCD, Intel Ring など)
list<list<int>> groups = sys.topology.cache_groups();

for (i in 0..length(groups)) {
    print("キャッシュグループ", i, ":", groups[i]);
};
```

### 並列処理の最適化パターン

```dri
use sys;

# parallel for 実行前に L3 共有コアにスレッドを集中
sys.topology.pin_threads();   # 自動で最適キャッシュグループに固定

int[] data = range(1000000);
int total = 0;

parallel for reduction(+:total) (x of data) {
    total += x * x;
};

print("結果:", total);
```

### sys.topology API

| 関数 | 戻り値型 | 説明 |
|------|--------|------|
| `sys.topology.p_cores()` | `list<int>` | 高性能 (P) コア ID のリスト |
| `sys.topology.e_cores()` | `list<int>` | 効率 (E) コア ID のリスト |
| `sys.topology.cache_groups()` | `list<list<int>>` | L3 キャッシュ共有グループ |
| `sys.topology.total()` | `int` | 全論理コア数 |
| `sys.topology.vendor()` | `String` | CPU 製造元の文字列 |
| `sys.topology.pin_threads()` | `void` | 現在のスレッドを最適グループに固定 |
| `sys.topology.set_affinity(n)` | `void` | 特定のコア n に現在のスレッドを固定 |

### 効果

- **Intel 第 12 世代以降**: 並列ループに P-コアのみを割り当て → E-コアの低速な周波数を回避
- **AMD Ryzen (マルチ CCD)**: 同一 CCD のコアにスレッドを集中 → クロス CCD レイテンシ (~80ns) を排除
- **一般**: L3 共有グループ内のみでスレッドを配置 → キャッシュミスが激減
