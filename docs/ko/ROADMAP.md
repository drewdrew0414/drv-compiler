# dri 로드맵

dri 컴파일러 및 언어 생태계의 개발 방향을 정리합니다.  
*마지막 업데이트: 2026-05-21 — v0.2 전체 + v0.3 일부 + v0.4 일부 + v0.5 일부 + v1.0 일부 적용*

---

## 현재 상태 (v0.1.0 — 트랜스파일러 MVP 출시)

C++20 트랜스파일러 파이프라인 전 구간(렉서 → 파서 → 정적 분석 → 코드 생성 → 백엔드 호출)이
동작하며, dri 소스를 실제로 빌드해 실행 파일을 생성할 수 있습니다.

| 영역 | 상태 |
|------|------|
| 기본 문법 (변수, 타입, 입출력) | ✅ 구현 완료 |
| 제어문 (if, for, while, switch, match) | ✅ 구현 완료 |
| 함수·클래스·상속·트레이트·impl | ✅ 구현 완료 |
| 람다 / `|>` 파이프 / 고차 함수 | ✅ 구현 완료 |
| 컬렉션 (`array`, `list`, `Map`, `tensor` 기본형) | ✅ 구현 완료 |
| 예외 처리 (`try/catch`, `Option`, `Result`, `match`) | ✅ 구현 완료 |
| 메모리 모델 (`Own`, `Ref`, `Borrow`, `move`, `@region`) | ✅ 구현 완료 |
| SIMD 루프 (`simd for`) + reduction | ✅ 구현 완료 (OpenMP `simd`) |
| 병렬 루프 (`parallel for`, `parallel simd for`) | ✅ 구현 완료 (OpenMP) |
| `async / await / spawn` | ✅ 구현 완료 (`std::async`/스레드 기반) |
| 어노테이션 (`@inline`, `@pure`, `@noalias`, `@threadsafe`, `@bench`, `@trace`, `@specialize`, `@fast_math`, `@stack/@heap`, `@warrant/@rebuttal/@defeats`) | ✅ 구현 완료 |
| `compile_eval` / `static_if` | ✅ 구현 완료 |
| 내장 함수 (str, lst, map, math, io, sys, diff.*) | ✅ 구현 완료 |
| 모듈 시스템 (`module` / `use`) | ✅ 1단계 모듈 로딩, 중복 import 억제 |
| `#line` 지시문 + 소스 맵 (JSON) | ✅ 구현 완료 |
| 정적 분석기 (스레드 안전성·앨리어싱·`@packed` 상속·`atomic<String>`) | ✅ 구현 완료 |
| 점진적 빌드 캐시 (CRC32 + mtime) | ✅ 구현 완료 |
| 크로스 컴파일 (`--target`, `--sysroot`, `--cross-cxx`) | ✅ 구현 완료 |
| 백엔드 (`clang++` / `g++`, `-fopenmp`, `-flto`, `-march=native`) | ✅ 구현 완료 |
| Windows / macOS / Linux 인스톨러 | ✅ 1차 배포 (PS1 + WinForms + Bash) |
| 런타임 안전성 (경계 검사, null 역참조, 빈 pop, 음수 fill) | ✅ v0.1.1에서 추가 |
| 예제 08 — 컴파일러 재배치 최적화 | ✅ 추가됨 |
| 예제 09 — Roofline 모델 벤치마크 | ✅ 추가됨 |

> **CLI 옵션 요약**: `--exe / --cpp / --check / --debug / --release / --opt N /
> --native / --lto / --trace / --incremental / --cache-dir /
> --no-analyze / --target / --sysroot / --cross-cxx / --source-map /
> --strict / --link <libs> / -D<FLAG>`

---

## v0.2 — 툴체인 안정화 (다음 마일스톤)

**목표**: 출시된 트랜스파일러를 실전에 견딜 수 있도록 다듬는다.

### 컴파일러 품질
- [x] 에러 메시지 통일 — 모든 단계가 `파일:줄:열: 카테고리: 메시지` 포맷 준수
- [x] 진단 메시지 한국어/영어/일본어 i18n 옵션 (`--lang ko|en|ja`)
- [ ] `--source-map` 출력 검증 도구
- [x] 회귀 테스트 스위트 (`tests/cases/*.dri` + `tests/run_tests.sh`, 21개 테스트)
- [x] CI 매트릭스 (`.github/workflows/ci.yml` — Linux/macOS × clang/g++)

### 빌드 시스템
- [x] `dri build` 멀티 파일 프로젝트 매니페스트 (`dri.drpm`) — `name/version/main/output/link/search_dirs`
- [x] 시스템 정적 라이브러리 링크 (`--link math,m,pthread`)
- [x] 인스톨러 자동 서명 (Windows sign.ps1 EV 인증서 감지 포함)
- [ ] `installer/linux/build-installer.sh` 결과물을 R2 레지스트리에 자동 업로드

### 안전성
- [x] `--strict` 모드 (모든 경고를 에러로 승격)
- [x] 정수 오버플로 검사 (`@checked_arith` 어노테이션)
- [x] 모듈 import 사이클 감지 (DFS 기반, 체인 경로 표시)

---

## v0.3 — 타입 시스템 강화

**목표**: 제네릭과 소유권을 단순 통과 매핑이 아닌, 실제로 *검사*되는 시스템으로.

- [ ] 진짜 제네릭 함수·클래스 (현재는 C++ 템플릿으로 전달만 함)
- [ ] 트레이트 바운드 검증 (`T implements Printable` → 구현체 존재 확인)
- [x] 소유권 흐름 분석 (`move` 이후 사용 시 컴파일 에러 — use-after-move 감지)
- [ ] `Borrow<T>` 수명 검사 (escape analysis)
- [ ] `dim` 물리 단위 타입 충돌 검사 (`@units_check`)
- [ ] 리플렉션 (`reflect.type_of`, `reflect.fields`)
- [x] `Option<T>` / `Result<T>` 누락 처리 경고 — 미처리 반환값 warn

---

## v0.4 — HPC 런타임 확장

**목표**: 단순 OpenMP를 넘어, 실제 HPC 워크로드에 최적화된 런타임.

- [x] AVX-512 자동 디스패치 (`@avx512` → 런타임 디스패처 + avx512f 클론)
- [ ] Work-Stealing 스레드 풀 (`parallel for of` 컬렉션용)
- [x] `tensor<N, T>` 고정 차원 SIMD 최적화 배열 — `std::array<T,N>`으로 매핑
- [x] `simd.fmadd/fmsub/fnmadd`, `math.invsqrt`, `bits.popcount/clz/ctz/bswap/rotl/rotr/log2` 인트린식 매핑
- [x] `mem.prefetch/fence/zero`, `sys.affinity/time_ms/cpu_count`, `sys.sync.*` 메모리 어노테이션
- [ ] 비차단 I/O (`async io.read_file` → `io_uring` / `kqueue` / `IOCP`)
- [x] `--trace` 결과를 Chrome `tracing` 포맷으로 출력 (μs 단위, `ts`/`dur` 숫자 타입, `@bench` 포함)

---

## v0.5 — 컴파일 타임 메타프로그래밍

**목표**: 사용자가 직접 컴파일 타임에 로직을 표현할 수 있는 기반.

- [x] `diff.forward` / `diff.numerical` / `diff.hessian` 자동 미분 코드 생성
- [x] `@bench` 측정 결과를 JSON에 첨부 (`--bench-json <file>`)
- [x] `@specialize<T=float, double>` 타입별 SIMD 특수화 생성 (명시적 템플릿 인스턴스화)
- [ ] `static_if` 안에서 사용 가능한 컴파일 타임 함수 표준 라이브러리
- [ ] `@warrant / @rebuttal / @defeats` 메타데이터를 ELF/Mach-O 섹션으로 임베드
- [ ] `extern "FFI"` 모던 언어 안전 연동 (Rust, Zig → `Own/Ref` 자동 매핑)
- [ ] `@unsafe_legacy extern "C"` 레거시 방어 계층 (시그널 핸들러 자동 주입)

---

## v0.6 — 외부 라이브러리 생태계

**목표**: dri-pandas, dri-toulmin, dri-data, dri-plot 핵심 4종 라이브러리 구현.

- [ ] **dri-pandas** — 데이터프레임 파이프라인
  - [ ] Apache Arrow / Rust Polars 코어 FFI 래핑
  - [ ] `.query()`, `.assign()`, `.groupby()`, `.mean()` 메서드 체이닝
  - [ ] `df.to_tensor()` 텐서 변환
- [ ] **dri-toulmin** — 논증 룰 엔진 ([참조](https://github.com/park-jun-woo/toulmin))
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

## v1.0 — 패키지 관리자 + 자체 호스팅

**목표**: 실용적인 dri 생태계 완성과 자체 호스팅 컴파일러.

- [ ] `dri` 패키지 관리자
  - [x] `dri init` — 프로젝트 초기화 (dri.drpm + src/ + tests/ 스캐폴딩)
  - [ ] `dri install` — SemVer + lock 파일 의존성 설치
  - [ ] `dri publish` — R2 레지스트리 배포
  - [x] `dri build` / `dri test` — 태스크 실행 (build는 드리 매니페스트 빌드, test는 tests/ 실행)
- [ ] 표준 라이브러리 패키지 (`math`, `tensor`, `http`, `json`)
- [ ] LSP 서버 — IDE 자동 완성·진단·hover (`dri-lsp`)
- [ ] Playground — 브라우저에서 dri 실행 (WASM 컴파일러 + WASI 런타임)
- [ ] 자체 호스팅 (dri로 작성된 dri 컴파일러 1차 부트스트랩)

---

## 장기 계획 (v1.1+)

| 계획 | 설명 |
|------|------|
| LLVM 백엔드 | C++ 트랜스파일을 거치지 않고 LLVM IR 직접 생성, 더 나은 최적화 |
| 점진적 컴파일 (모듈) | `*.dri.bc` 캐싱으로 변경된 모듈만 재컴파일 |
| WASM 타깃 | WebAssembly 출력 (Playground 백엔드와 공유) |
| GPU 커널 | CUDA / SPIR-V / Metal 백엔드 (`@gpu parallel for`) |
| 대화형 REPL | 즉석 dri 실행 환경 (LSP 기반) |
| 인터프리터 모드 | 스크립팅 용도 (`dri --run script.dri`) |
| 디버거 통합 | DWARF 라인 매핑 + gdb/lldb 사용자 정의 프린터 |

---

## 기여

로드맵의 특정 항목에 기여하고 싶다면 이슈를 열어 주세요.  
기여 방법은 [CONTRIBUTING.md](CONTRIBUTING.md)를 참고하세요.

---
