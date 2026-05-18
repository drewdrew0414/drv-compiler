# 예제 1: SIMD 가속 행렬 곱셈

> HPC에서 가장 기초적이고 중요한 연산. SIMD와 병렬화를 동시에 적용한다.

---

## 개요

- N×N 행렬 두 개를 곱하는 O(N³) 연산
- `simd for` + `parallel for` 조합으로 CPU SIMD 레인과 멀티코어를 동시 활용
- `@align(64)` 로 캐시 라인 정렬, `@noalias` 로 포인터 별칭 방지
- `where`/`otherwise` 마스크로 경계 조건 처리

---

## 코드

```dri
# 행렬 곱셈: C = A × B  (N×N)
# SIMD + 병렬 + 캐시 최적화

module matmul;

int N = 512;

# 정렬된 2D 배열 (평탄화)
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

# 무작위 행렬 초기화 (시드 기반)
void fill_sequential(Matrix m, int offset) {
    for (i in 0..m.rows) {
        for (j in 0..m.cols) {
            m.set(i, j, (i * m.cols + j + offset) * 0.001);
        }
    }
}

# 순수 C 스타일 행렬 곱 (기준선)
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

# SIMD + 병렬 최적화 행렬 곱
# 루프 순서를 i-k-j (캐시 친화적)로 변경
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
                    # 0 곱셈 skip — SIMD 마스크로 처리
                }
            }
        }
    }
}

# 결과 검증 (일부 원소만)
boolean verify(Matrix c_ref, Matrix c_opt, double tol) {
    for (i in 0..10) {
        for (j in 0..10) {
            double diff = c_ref.get(i, j) - c_opt.get(i, j);
            if (diff < 0.0) { diff = -diff; }
            if (diff > tol) {
                print("불일치 발견:", i, j, c_ref.get(i, j), c_opt.get(i, j));
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

# 기준선 측정
double t0 = perf.now();
matmul_naive(a, b, c1);
double naive_ms = perf.now() - t0;
print(`naive:  ${naive_ms} ms`);

# SIMD+병렬 측정 (@bench 어노테이션이 내부 타이머 출력)
matmul_simd_parallel(a, b, c2);

# 검증
boolean ok = verify(c1, c2, 1e-9);
print(`검증: ${ok}`);

# GFLOP/s 계산
double ops = 2.0 * N * N * N;           # N³ mul + N³ add
double gflops = ops / (naive_ms * 1e6); # ms → seconds
print(`이론 GFLOP/s: ${gflops}`);
```

---

## 학습 포인트

| 기법 | 설명 |
|------|------|
| `@align(64)` | CPU 캐시 라인(64바이트)에 데이터 정렬 |
| `i-k-j` 루프 순서 | B 행렬을 순차 접근 → 캐시 히트율 극대화 |
| `simd for` + `where` | 0 곱셈을 SIMD 마스크로 skip |
| `parallel for` 외층 | 행 단위로 스레드 분배 |
| `@bench` | 함수 실행 시간 자동 출력 |

---

## 기대 출력

```
[bench] matmul_simd_parallel: XX.XXX ms
naive:  XXXX.XXX ms
검증: true
이론 GFLOP/s: X.XX
```
