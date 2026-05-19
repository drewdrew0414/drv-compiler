# SIMD と高性能計算
> `simd for`、`tensor`、`simd.fmadd`、`math.invsqrt` などで CPU ベクトル演算を最大限に活用します。

---

## SIMD 最適化のヒント

| 手法 | 効果 |
|------|------|
| `simd for` | 基本ベクトル化 |
| `@noalias` | より積極的なベクトル化を許可 |
| `mem.prefetch` | キャッシュミスの低減 |
| `simd.fmadd` | FMA による精度・速度の向上 |
| `math.invsqrt` | 逆平方根の高速化 |
| `sys.affinity` | NUMA/キャッシュ効率の最適化 |

---

## simd for

```
simd for (変数 in 開始..終了) { 本体 };
```

SIMD 命令を自動生成します。
x86-64: AVX2/AVX-512 | ARM: NEON

```dri
int N = 1024;
double[] x = new double[N];
double[] y = new double[N];
double[] z = new double[N];

simd for (i in 0..N) {
    z[i] = x[i] * 2.0 + y[i];
};
```

`@noalias` でエイリアシングがないことを宣言すると、より積極的なベクトル化が可能になります:

```dri
@noalias
void vec_add(double[] a, double[] b, double[] c, int n) {
    simd for (i in 0..n) {
        c[i] = a[i] + b[i];
    };
};
```

---

## tensor 演算

`tensor<N, T>` — 静的サイズで SIMD 最適化された配列

```dri
tensor<8, double> v = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
tensor<8, double> u = {2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0};

print(sum(v));      # 36.0
print(dot(v, u));   # 72.0
print(norm(v));     # ユークリッドノルム = sqrt(204)
```

---

## 数学組み込み関数 (直接呼び出し)

```dri
tensor<5, double> data = {3.0, 1.0, 4.0, 1.0, 5.0};

print(sum(data));          # 合計
print(mean(data));         # 平均
print(std(data));          # 標準偏差
print(median(data));       # 中央値
print(min_index(data));    # 最小値のインデックス
print(max_index(data));    # 最大値のインデックス
print(dot(data, data));    # 内積
print(norm(data));         # ノルム
```

| 関数 | 説明 |
|------|------|
| `sum(t)` | 要素の合計 (AVX 自動ディスパッチ) |
| `mean(t)` | 平均 |
| `std(t)` | 標準偏差 |
| `median(t)` | 中央値 |
| `dot(a, b)` | 内積 |
| `norm(t)` | ユークリッドノルム |
| `min_index(t)` | 最小値のインデックス |
| `max_index(t)` | 最大値のインデックス |

---

## simd.fmadd — Fused Multiply-Add

`a * b + c` を 1 回の SIMD 命令で計算します。精度と速度を同時に向上させます。

```dri
double result = simd.fmadd(3.0, 4.0, 5.0);   # 17.0

# 高速な内積計算
double dot_product(double[] a, double[] b, int n) {
    double s = 0.0;
    for (i in 0..n) {
        s = simd.fmadd(a[i], b[i], s);
    };
    return s;
};
```

---

## math.invsqrt — 逆平方根

`1 / sqrt(x)` の高速計算。ベクトル正規化に有用です。

```dri
double inv = math.invsqrt(16.0);   # 0.25

void normalize(double[] v, int n) {
    double sq_sum = 0.0;
    for (i in 0..n) {
        sq_sum = simd.fmadd(v[i], v[i], sq_sum);
    };
    double inv_norm = math.invsqrt(sq_sum);
    simd for (i in 0..n) {
        v[i] = v[i] * inv_norm;
    };
};
```

---

## bits.popcount — ビット数

```dri
int n = 0b10110101;
int cnt = bits.popcount(n);   # 5

int hamming(int a, int b) {
    return bits.popcount(a ^ b);
};

print(hamming(0b1010, 0b1100));   # 2
```

---

## mem.prefetch — キャッシュプリフェッチ

次に使用するデータを事前にキャッシュに読み込み、キャッシュミスを減らします。

```dri
double[] large = new double[1000000];

for (i in 0..1000000) {
    if (i + 64 < 1000000) {
        mem.prefetch(large[i + 64]);
    };
    process(large[i]);
};
```

---

## 高性能 I/O

```dri
# メモリマップファイル読み込み (コピーなしで直接アクセス)
var mapped = io.mmap_read("huge_dataset.bin");

# 高速 CSV パーサ (SIMD ベース)
var table = fast_csv_read("data.csv");
```

---

## 性能計測

```dri
double t0 = perf.now();

simd for (i in 0..1000000) {
    result[i] = a[i] * b[i] + c[i];
};

double t1 = perf.now();
double elapsed = t1 - t0;
print("所要:", elapsed, "ms");
```

---

## 全体例 — 信号処理

```dri
int N = 8192;
double[] signal = new double[N];
double[] kernel = new double[N];
double[] output = new double[N];

# 初期化
for (i in 0..N) {
    signal[i] = sin(i as double * 0.01);
    kernel[i] = cos(i as double * 0.01);
};

double t0 = perf.now();

# 点別乗算
@noalias
void hadamard(double[] a, double[] b, double[] c, int n) {
    simd for (i in 0..n) {
        c[i] = a[i] * b[i];
    };
};

hadamard(signal, kernel, output, N);

# エネルギー計算
double energy = 0.0;
for (i in 0..N) {
    energy = simd.fmadd(output[i], output[i], energy);
};

double t1 = perf.now();
print("エネルギー:", energy);
print("処理時間:", t1 - t0, "ms");

# 統計 (最初の 8 サンプル)
tensor<8, double> sample = {
    output[0], output[1], output[2], output[3],
    output[4], output[5], output[6], output[7]
};
print("平均:", mean(sample));
print("標準偏差:", std(sample));
```

> `parallel` と `simd for` を併用するパターンは [[11_parallelism.md](11_parallelism.md)] を参照してください。

---
