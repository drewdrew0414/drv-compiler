# 例 1: SIMD 加速行列積

> HPC において最も基礎的で重要な演算。SIMD と並列化を同時に適用する。

---

## 概要

- N×N の行列 2 つを掛ける O(N³) 演算
- `simd for` + `parallel for` の組み合わせで CPU SIMD レーンとマルチコアを同時活用
- `@align(64)` でキャッシュライン整列、`@noalias` でポインタエイリアシングを防止
- `where`/`otherwise` マスクで境界条件を処理

---

## コード

```dri
# 行列積: C = A × B  (N×N)
# SIMD + 並列 + キャッシュ最適化

module matmul;

int N = 512;

# 整列済み 2D 配列 (平坦化)
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

# 連番値で行列を初期化 (シードベース)
void fill_sequential(Matrix m, int offset) {
    for (i in 0..m.rows) {
        for (j in 0..m.cols) {
            m.set(i, j, (i * m.cols + j + offset) * 0.001);
        }
    }
}

# 素朴な C スタイルの行列積 (ベースライン)
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

# SIMD + 並列最適化行列積
# ループ順序を i-k-j (キャッシュ友好的) に変更
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
                    # ゼロ乗算をスキップ — SIMD マスクで処理
                }
            }
        }
    }
}

# 結果の検証 (先頭 10×10 要素のみ)
boolean verify(Matrix c_ref, Matrix c_opt, double tol) {
    for (i in 0..10) {
        for (j in 0..10) {
            double diff = c_ref.get(i, j) - c_opt.get(i, j);
            if (diff < 0.0) { diff = -diff; }
            if (diff > tol) {
                print("不一致発見:", i, j, c_ref.get(i, j), c_opt.get(i, j));
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

# ベースライン計測
double t0 = perf.now();
matmul_naive(a, b, c1);
double naive_ms = perf.now() - t0;
print(`naive:  ${naive_ms} ms`);

# SIMD+並列計測 (@bench アノテーションが内部タイマーを出力)
matmul_simd_parallel(a, b, c2);

# 検証
boolean ok = verify(c1, c2, 1e-9);
print(`検証: ${ok}`);

# GFLOP/s 計算
double ops = 2.0 * N * N * N;           # N³ mul + N³ add
double gflops = ops / (naive_ms * 1e6); # ms → 秒
print(`理論 GFLOP/s: ${gflops}`);
```

---

## 学習ポイント

| 技法 | 説明 |
|------|------|
| `@align(64)` | CPU キャッシュライン (64 バイト) にデータを整列 |
| `i-k-j` ループ順序 | 行列 B への順次アクセス → キャッシュヒット率の最大化 |
| `simd for` + `where` | ゼロ乗算を SIMD マスクでスキップ |
| 外側の `parallel for` | 行単位でスレッドに分配 |
| `@bench` | 関数の実行時間を自動出力 |

---

## 期待される出力

```
[bench] matmul_simd_parallel: XX.XXX ms
naive:  XXXX.XXX ms
検証: true
理論 GFLOP/s: X.XX
```
