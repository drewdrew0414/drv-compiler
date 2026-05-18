# dri 로드맵

dri 컴파일러 및 언어 생태계의 개발 방향을 정리합니다.

---

## 현재 상태 (v0.1 — 초기 프로토타입)

| 기능 | 상태 |
|------|------|
| 기본 문법 (변수, 타입, 입출력) | ✅ 설계 완료 |
| 제어문 (if, for, while, switch, match) | ✅ 설계 완료 |
| 함수·클래스·상속 | ✅ 설계 완료 |
| 트레이트·impl | ✅ 설계 완료 |
| 컬렉션 (array, list, Map, tensor) | ✅ 설계 완료 |
| 예외 처리 (try/catch, Option, Result) | ✅ 설계 완료 |
| 함수형 프로그래밍 (람다, map/filter/reduce, 파이프) | ✅ 설계 완료 |
| 메모리 모델 (Own, Ref, Borrow, @region) | ✅ 설계 완료 |
| SIMD (`simd for`, `simd.fmadd`, tensor 연산) | ✅ 설계 완료 |
| 병렬 처리 (`parallel`, `async/await`, `spawn`) | ✅ 설계 완료 |
| 어노테이션 시스템 | ✅ 설계 완료 |
| 내장 함수 (str, lst, map, math, io, sys) | ✅ 설계 완료 |
| C++ 트랜스파일러 | 🔧 개발 중 |
| 타입 추론·검사기 | 🔧 개발 중 |

---

## v0.2 — 컴파일러 기반 구축

**목표**: 기본 dri 프로그램을 C++17로 트랜스파일하고 실행

### 컴파일러 파이프라인

- [ ] Lexer — 토큰화
  - [ ] 키워드·연산자·리터럴 인식
  - [ ] 멀티라인 주석 `!# ... ##`
  - [ ] 문자열 보간 `` `{변수}` ``
- [ ] Parser — AST 구성
  - [ ] 표현식·문장·선언 파싱
  - [ ] 제어문 (if / for / while / switch / match)
  - [ ] 함수·클래스 선언
- [ ] 타입 검사기
  - [ ] 기본 타입 추론 (`var`)
  - [ ] 함수 시그니처 검사
  - [ ] 트레이트 구현 검증
- [ ] C++ 코드 생성
  - [ ] 기본 문법 → C++17 변환
  - [ ] 컬렉션 → STL 변환
  - [ ] `#line` 지시문으로 소스 맵 생성

### 빌드 시스템

- [ ] `comp` CLI 기본 동작 (`--exe`, `--check`, `--debug`, `--release`)
- [ ] CMake 통합

---

## v0.3 — 타입 시스템 완성

**목표**: 제네릭, 소유권 모델, 단위 타입 지원

- [ ] 제네릭 함수·클래스 (`T identity<T>(T x)`)
- [ ] 소유권 타입 검사 (`Own<T>`, `move`, `Borrow<T>`)
- [ ] `Option<T>` / `Result<T>` / `match` 표현식
- [ ] `dim` 물리 단위 타입 + `@units_check`
- [ ] 리플렉션 (`reflect.type_of`, `reflect.fields`)
- [ ] `compile_eval` 컴파일 타임 상수 계산

---

## v0.4 — HPC 런타임

**목표**: SIMD·병렬 기능이 실제로 동작

- [ ] `simd for` → AVX2/AVX-512 자동 벡터화
- [ ] `parallel for (x of ...)` → Work-Stealing 스레드 풀
- [ ] `parallel simd for (i in 0..N)` → 병렬 + 벡터화 조합
- [ ] `tensor<N, T>` SIMD 최적화 배열
- [ ] `simd.fmadd`, `math.invsqrt`, `bits.popcount`
- [ ] `mem.prefetch`, `sys.affinity`
- [ ] `@noalias`, `@stack`, `@heap`, `@region` 메모리 어노테이션
- [ ] `async/await` 비동기 I/O
- [ ] `spawn` 파이어 앤 포겟 태스크

---

## v0.5 — 언어 고급 기능

**목표**: 자동 미분, 고급 최적화 어노테이션, 분기 추적, 툴민 논증 어노테이션 지원

- [ ] `diff.forward` / `diff.numerical` / `diff.hessian` 자동 미분
- [ ] `@bench` 함수 실행 시간 자동 측정
- [ ] `@specialize` 타입별 SIMD 특수화
- [ ] `static_if` 컴파일 타임 조건 분기
- [ ] `module/use` 모듈 시스템 (순환 의존 감지)
- [ ] `--trace` parallel 루프 시각화 (Chrome DevTools 형식)
- [ ] `@trace` 분기 추적 인스트루멘테이션 자동 주입
- [ ] `sys.get_branch_trace()` / `sys.clear_branch_trace()` 런타임 내장 함수
- [ ] `@warrant`, `@rebuttal`, `@defeats` 툴민 논증 어노테이션 파싱 및 바이너리 메타데이터 포함
- [ ] `extern "FFI"` 모던 언어 안전 연동 (Rust, Zig → `Own/Ref` 자동 매핑)
- [ ] `@unsafe_legacy extern "C"` 레거시 방어 계층 (시그널 핸들러 자동 주입)

---

## v0.6 — 외부 라이브러리 생태계

**목표**: dri-pandas, dri-toulmin, dri-data, dri-plot 핵심 4종 라이브러리 구현

- [ ] **dri-pandas** — 데이터프레임 파이프라인
  - [ ] Apache Arrow / Rust Polars 코어 FFI 래핑
  - [ ] `.query()`, `.assign()`, `.groupby()`, `.mean()` 메서드 체이닝
  - [ ] `df.to_tensor()` 텐서 변환
- [ ] **dri-toulmin** — 논증 룰 엔진 ([참조: park-jun-woo/toulmin](https://github.com/park-jun-woo/toulmin))
  - [ ] 신뢰도 계산: $Acc(a) = \frac{w(a)}{1 + \sum Acc(attackers)}$
  - [ ] `toulmin.load_rules()` 바이너리 메타데이터 로드
  - [ ] `toulmin.build_audit()` 감사 추적 트리 생성
- [ ] **dri-data** — 멀티 포맷 입출력
  - [ ] CSV, JSON, XML 고속 직렬화
  - [ ] DataFrame 테이블 → PNG 이미지 내보내기
  - [ ] 논증 트리 → PNG 이미지 내보내기
- [ ] **dri-plot** — 데이터 시각화
  - [ ] line, bar, scatter, heatmap 차트 엔진
  - [ ] `plot.dashboard()` 대시보드 구성
  - [ ] GUI 창(`show()`) 및 HTML/PNG 내보내기

---

## v1.0 — 패키지 관리자 및 생태계

**목표**: 실용적인 dri 생태계 완성

- [ ] `drvpm` 패키지 관리자
  - [ ] `drvpm init` 프로젝트 초기화
  - [ ] `drvpm install` 의존성 설치 (SemVer + lock 파일)
  - [ ] `drvpm publish` R2 레지스트리 배포
  - [ ] `drvpm build/test/run` 태스크 실행
- [ ] 표준 라이브러리 패키지 (`math`, `tensor`, `http`, `json`)
- [ ] LLVM 백엔드 (C++ 트랜스파일 없이 직접 LLVM IR 생성)
- [ ] LSP 서버 (IDE 자동완성·진단)
- [ ] Playground (브라우저에서 dri 실행)

---

## 장기 계획

| 계획 | 설명 |
|------|------|
| LLVM 백엔드 | 직접 네이티브 코드 생성, 더 나은 최적화 |
| 점진적 컴파일 | 변경된 파일만 재컴파일 |
| WASM 타깃 | WebAssembly 출력 지원 |
| GPU 커널 | CUDA/OpenCL 백엔드 |
| 대화형 REPL | 즉석 dri 실행 환경 |
| 인터프리터 모드 | 스크립팅 용도 |

---

## 기여

로드맵의 특정 항목에 기여하고 싶다면 이슈를 열어 주세요.  
기여 방법은 [CONTRIBUTING.md](CONTRIBUTING.md)를 참고하세요.

---
