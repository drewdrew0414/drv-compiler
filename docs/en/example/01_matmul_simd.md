# Example 1: SIMD-Accelerated Matrix Multiplication

> One of the most fundamental and important operations in HPC. Applies SIMD and parallelism simultaneously.

---

## Overview

- O(N³) operation: multiply two N×N matrices
- Combines `simd for` + `parallel for` to exploit both CPU SIMD lanes and multi-core
- `@align(64)` for cache-line alignment, `@noalias` to prevent pointer aliasing
- `where`/`otherwise` masks handle boundary conditions

---

## Code

```dri
# Matrix multiplication: C = A × B  (N×N)
# SIMD + parallel + cache optimization

module matmul;

int N = 512;

# aligned 2D array (flattened)
@align(64)
class Matrix {
    int rows;
    int cols;
    double[] data;

    void init(int r, int c) {
        rows = r;
        cols = c;
        data = lst.fill(r * c, 0.0);
    }

    double get(int r, int c) {
        return data[r * cols + c];
    }

    void set(int r, int c, double v) {
        data[r * cols + c] = v;
    }
}

# initialize matrix with sequential values (seed-based)
void fill_sequential(Matrix m, int offset) {
    for (i in 0..m.rows) {
        for (j in 0..m.cols) {
            m.set(i, j, (i * m.cols + j + offset) * 0.001);
        }
    }
}

# plain C-style matrix multiply (baseline)
void matmul_naive(Matrix a, Matrix b, Matrix c) {
    for (i in 0..N) {
        for (j in 0..N) {
            double s = 0.0;
            for (k in 0..N) {
                s += a.get(i, k) * b.get(k, j);
            }
            c.set(i, j, s);
        }
    }
}

# SIMD + parallel optimized matrix multiply
# loop order changed to i-k-j (cache-friendly)
@bench
void matmul_simd_parallel(Matrix a, Matrix b, Matrix c) {
    parallel for (i in 0..N) {
        for (k in 0..N) {
            double aik = a.get(i, k);
            simd for (j in 0..N) {
                where (aik != 0.0) {
                    double old_val = c.get(i, j);
                    c.set(i, j, old_val + aik * b.get(k, j));
                } otherwise {
                    # zero multiply — skip via SIMD mask
                }
            }
        }
    }
}

# verify result (spot-check first 10×10 elements)
boolean verify(Matrix c_ref, Matrix c_opt, double tol) {
    for (i in 0..10) {
        for (j in 0..10) {
            double diff = c_ref.get(i, j) - c_opt.get(i, j);
            if (diff < 0.0) { diff = -diff; }
            if (diff > tol) {
                print("mismatch at:", i, j, c_ref.get(i, j), c_opt.get(i, j));
                return false;
            }
        }
    }
    return true;
}

Matrix a;  a.init(N, N);
Matrix b;  b.init(N, N);
Matrix c1; c1.init(N, N);
Matrix c2; c2.init(N, N);

fill_sequential(a, 0);
fill_sequential(b, 100);

# baseline timing
double t0 = perf.now();
matmul_naive(a, b, c1);
double naive_ms = perf.now() - t0;
print(`naive:  ${naive_ms} ms`);

# SIMD+parallel timing (@bench annotation prints internal timer)
matmul_simd_parallel(a, b, c2);

# verify
boolean ok = verify(c1, c2, 1e-9);
print(`verified: ${ok}`);

# GFLOP/s estimate
double ops = 2.0 * N * N * N;           # N³ mul + N³ add
double gflops = ops / (naive_ms * 1e6); # ms → seconds
print(`theoretical GFLOP/s: ${gflops}`);
```

---

## Key Takeaways

| Technique | Description |
|-----------|-------------|
| `@align(64)` | Aligns data to CPU cache line (64 bytes) |
| `i-k-j` loop order | Sequential access of matrix B → maximises cache hit rate |
| `simd for` + `where` | Skip zero multiplies via SIMD mask |
| Outer `parallel for` | Distributes rows across threads |
| `@bench` | Automatically prints function execution time |

---

## Expected Output

```
[bench] matmul_simd_parallel: XX.XXX ms
naive:  XXXX.XXX ms
verified: true
theoretical GFLOP/s: X.XX
```
