# SIMD와 고성능 컴퓨팅
> `simd for`, `tensor`, `simd.fmadd`, `math.invsqrt` 등으로 CPU 벡터 연산을 최대한 활용한다.

---

## SIMD 최적화 팁

| 기법 | 효과 |
|------|------|
| `simd for` | 기본 벡터화 |
| `@noalias` | 더 공격적인 벡터화 허용 |
| `mem.prefetch` | 캐시 미스 감소 |
| `simd.fmadd` | FMA로 정밀도·속도 향상 |
| `math.invsqrt` | 역제곱근 고속화 |
| `sys.affinity` | NUMA/캐시 효율 최적화 |

---

## simd for

```
simd for (변수 in 시작..끝) { 본문 };
```

SIMD 명령어를 자동 생성합니다.  
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

`@noalias`로 앨리어싱 없음을 선언하면 더 적극적인 벡터화가 가능합니다:

```dri
@noalias
void vec_add(double[] a, double[] b, double[] c, int n) {
    simd for (i in 0..n) {
        c[i] = a[i] + b[i];
    };
};
```

---

## tensor 연산

`tensor<N, T>` — 정적 크기, SIMD 최적화된 배열

```dri
tensor<8, double> v = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
tensor<8, double> u = {2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0};

print(sum(v));      # 36.0
print(dot(v, u));   # 72.0
print(norm(v));     # 유클리드 노름 = sqrt(204)
```

---

## 수학 내장 함수 (직접 호출)

```dri
tensor<5, double> data = {3.0, 1.0, 4.0, 1.0, 5.0};

print(sum(data));          # 합계
print(mean(data));         # 평균
print(std(data));          # 표준편차
print(median(data));       # 중앙값
print(min_index(data));    # 최솟값 인덱스
print(max_index(data));    # 최댓값 인덱스
print(dot(data, data));    # 내적
print(norm(data));         # 노름
```

| 함수 | 설명 |
|------|------|
| `sum(t)` | 원소 합 (AVX 자동 디스패치) |
| `mean(t)` | 평균 |
| `std(t)` | 표준편차 |
| `median(t)` | 중앙값 |
| `dot(a, b)` | 내적 |
| `norm(t)` | 유클리드 노름 |
| `min_index(t)` | 최솟값 인덱스 |
| `max_index(t)` | 최댓값 인덱스 |

---

## simd.fmadd — Fused Multiply-Add

`a * b + c`를 한 번의 SIMD 명령으로 계산합니다. 정밀도와 속도를 동시에 향상시킵니다.

```dri
double result = simd.fmadd(3.0, 4.0, 5.0);   # 17.0

# 고속 내적 계산
double dot_product(double[] a, double[] b, int n) {
    double s = 0.0;
    for (i in 0..n) {
        s = simd.fmadd(a[i], b[i], s);
    };
    return s;
};
```

---

## math.invsqrt — 역제곱근

`1 / sqrt(x)` 고속 계산. 벡터 정규화에 유용합니다.

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

## bits.popcount — 비트 개수

```dri
int n = 0b10110101;
int cnt = bits.popcount(n);   # 5

int hamming(int a, int b) {
    return bits.popcount(a ^ b);
};

print(hamming(0b1010, 0b1100));   # 2
```

---

## mem.prefetch — 캐시 프리페치

다음에 사용할 데이터를 미리 캐시에 로딩해 캐시 미스를 줄입니다.

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

## 고성능 I/O

```dri
# 메모리 맵 파일 읽기 (복사 없이 직접 접근)
var mapped = io.mmap_read("huge_dataset.bin");

# 고속 CSV 파서 (SIMD 기반)
var table = fast_csv_read("data.csv");
```

---

## 성능 측정

```dri
double t0 = perf.now();

simd for (i in 0..1000000) {
    result[i] = a[i] * b[i] + c[i];
};

double t1 = perf.now();
double elapsed = t1 - t0;
print("소요:", elapsed, "ms");
```

---

## 전체 예시 — 신호 처리

```dri
int N = 8192;
double[] signal = new double[N];
double[] kernel = new double[N];
double[] output = new double[N];

# 초기화
for (i in 0..N) {
    signal[i] = sin(i as double * 0.01);
    kernel[i] = cos(i as double * 0.01);
};

double t0 = perf.now();

# 점별 곱셈
@noalias
void hadamard(double[] a, double[] b, double[] c, int n) {
    simd for (i in 0..n) {
        c[i] = a[i] * b[i];
    };
};

hadamard(signal, kernel, output, N);

# 에너지 계산
double energy = 0.0;
for (i in 0..N) {
    energy = simd.fmadd(output[i], output[i], energy);
};

double t1 = perf.now();
print("에너지:", energy);
print("처리 시간:", t1 - t0, "ms");

# 통계 (첫 8개 샘플)
tensor<8, double> sample = {
    output[0], output[1], output[2], output[3],
    output[4], output[5], output[6], output[7]
};
print("평균:", mean(sample));
print("표준편차:", std(sample));
```

> `parallel`과 `simd for`를 함께 활용하는 패턴은 [[11_병렬처리.md](11_병렬처리.md)]를 참고하세요.

---
