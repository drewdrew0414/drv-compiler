# SIMD and High-Performance Computing
> Use `simd for`, `tensor`, `simd.fmadd`, `math.invsqrt`, and friends to make the most of CPU vector operations.

---

## SIMD Optimization Tips

| Technique | Effect |
|------|------|
| `simd for` | Basic vectorization |
| `@noalias` | Allows more aggressive vectorization |
| `mem.prefetch` | Reduces cache misses |
| `simd.fmadd` | Improves precision and speed via FMA |
| `math.invsqrt` | Fast inverse square root |
| `sys.affinity` | NUMA/cache efficiency optimization |

---

## simd for

```
simd for (variable in start..end) { body };
```

Generates SIMD instructions automatically.
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

Declaring no aliasing with `@noalias` enables more aggressive vectorization:

```dri
@noalias
void vec_add(double[] a, double[] b, double[] c, int n) {
    simd for (i in 0..n) {
        c[i] = a[i] + b[i];
    };
};
```

---

## tensor Operations

`tensor<N, T>` — statically sized, SIMD-optimized array

```dri
tensor<8, double> v = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
tensor<8, double> u = {2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0};

print(sum(v));      # 36.0
print(dot(v, u));   # 72.0
print(norm(v));     # Euclidean norm = sqrt(204)
```

---

## Built-in Math Functions (Direct Calls)

```dri
tensor<5, double> data = {3.0, 1.0, 4.0, 1.0, 5.0};

print(sum(data));          # sum
print(mean(data));         # mean
print(std(data));          # standard deviation
print(median(data));       # median
print(min_index(data));    # index of the minimum
print(max_index(data));    # index of the maximum
print(dot(data, data));    # dot product
print(norm(data));         # norm
```

| Function | Description |
|------|------|
| `sum(t)` | element sum (auto-dispatched to AVX) |
| `mean(t)` | mean |
| `std(t)` | standard deviation |
| `median(t)` | median |
| `dot(a, b)` | dot product |
| `norm(t)` | Euclidean norm |
| `min_index(t)` | index of the minimum |
| `max_index(t)` | index of the maximum |

---

## simd.fmadd — Fused Multiply-Add

Computes `a * b + c` in a single SIMD instruction. Improves both precision and speed simultaneously.

```dri
double result = simd.fmadd(3.0, 4.0, 5.0);   # 17.0

# fast dot product
double dot_product(double[] a, double[] b, int n) {
    double s = 0.0;
    for (i in 0..n) {
        s = simd.fmadd(a[i], b[i], s);
    };
    return s;
};
```

---

## math.invsqrt — Inverse Square Root

Fast `1 / sqrt(x)` computation. Useful for vector normalization.

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

## bits.popcount — Bit Count

```dri
int n = 0b10110101;
int cnt = bits.popcount(n);   # 5

int hamming(int a, int b) {
    return bits.popcount(a ^ b);
};

print(hamming(0b1010, 0b1100));   # 2
```

---

## mem.prefetch — Cache Prefetch

Pre-loads data that will be used next into the cache to reduce cache misses.

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

## High-Performance I/O

```dri
# memory-mapped file read (direct access without copying)
var mapped = io.mmap_read("huge_dataset.bin");

# fast CSV parser (SIMD-based)
var table = fast_csv_read("data.csv");
```

---

## Performance Measurement

```dri
double t0 = perf.now();

simd for (i in 0..1000000) {
    result[i] = a[i] * b[i] + c[i];
};

double t1 = perf.now();
double elapsed = t1 - t0;
print("elapsed:", elapsed, "ms");
```

---

## Full Example — Signal Processing

```dri
int N = 8192;
double[] signal = new double[N];
double[] kernel = new double[N];
double[] output = new double[N];

# initialization
for (i in 0..N) {
    signal[i] = sin(i as double * 0.01);
    kernel[i] = cos(i as double * 0.01);
};

double t0 = perf.now();

# element-wise (Hadamard) product
@noalias
void hadamard(double[] a, double[] b, double[] c, int n) {
    simd for (i in 0..n) {
        c[i] = a[i] * b[i];
    };
};

hadamard(signal, kernel, output, N);

# energy calculation
double energy = 0.0;
for (i in 0..N) {
    energy = simd.fmadd(output[i], output[i], energy);
};

double t1 = perf.now();
print("energy:", energy);
print("processing time:", t1 - t0, "ms");

# statistics (first 8 samples)
tensor<8, double> sample = {
    output[0], output[1], output[2], output[3],
    output[4], output[5], output[6], output[7]
};
print("mean:", mean(sample));
print("std:", std(sample));
```

> For patterns combining `parallel` and `simd for`, see [[11_parallelism.md](11_parallelism.md)].

---
