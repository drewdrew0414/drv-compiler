# drv 기여 가이드

drv는 교육용 + HPC 시스템 언어를 목표로 하는 오픈소스 프로젝트입니다.

우리가 중요하게 생각하는 가치:

- **단순함** — 배우기 쉬운 문법
- **성능** — C++ 수준의 실행 속도
- **안전성** — 소유권·대여 모델, 타입 안전
- **교육 친화성** — 명확한 오류 메시지, 직관적인 개념
- **병렬 프로그래밍 접근성** — `parallel`, `simd for`, `async/await`를 쉽게

---

## 시작하기

### 저장소 클론

```bash
git clone https://github.com/drewdrew0414/drv-compiler.git
cd drv-compiler
```

### 빌드

```bash
cmake -B build
cmake --build build
```

### 테스트

```bash
cd build
ctest --output-on-failure
```

---

## 프로젝트 구조

```
drv-compiler/
├── main.cpp              # 컴파일러 진입점
├── compiler/
│   └── src/              # 컴파일러 소스 (Lexer, Parser, AST, Codegen)
├── docs/
│   ├── tutorial/         # 영문 튜토리얼
│   └── ko/
│       ├── education/    # 한국어 교육 문서
│       ├── CONTRIBUTING.md
│       ├── ROADMAP.md
│       └── SECURITY.md
├── CMakeLists.txt
└── LICENSE
```

---

## 기여 가능한 분야

### 컴파일러 코어

| 영역 | 내용 |
|------|------|
| Lexer | 토큰화, 키워드 추가 |
| Parser | 문법 규칙, AST 구성 |
| AST | 노드 최적화, 방문자 패턴 |
| 타입 시스템 | 타입 추론, 제네릭, 단위 타입 |
| C++ 코드 생성 | 트랜스파일 품질 개선 |

### HPC 런타임

| 영역 | 내용 |
|------|------|
| SIMD | AVX2/AVX-512/NEON 자동 벡터화 |
| 병렬 | Work-Stealing 스레드 풀 개선 |
| NUMA | 메모리 지역성 최적화 |
| tensor | 수치 연산 가속 |

### 언어 기능

| 기능 | 상태 |
|------|------|
| `diff.*` 자동 미분 | 설계 중 |
| `@units_check` 단위 타입 | 설계 중 |
| 패키지 관리자 `drvpm` | 설계 중 |
| LLVM 백엔드 | 미래 계획 |

### 문서

- 한국어 교육 문서 (`docs/ko/education/`) 보완
- 영문 튜토리얼 (`docs/tutorial/`) 추가
- 예제 코드 작성

---

## 기여 절차

1. 이슈를 먼저 열어 작업 내용을 공유합니다.
2. `main` 브랜치에서 분기합니다.
   ```bash
   git checkout -b feat/my-feature
   ```
3. 변경사항을 커밋합니다.
   ```bash
   git commit -m "feat: 기능 설명"
   ```
4. Pull Request를 생성합니다.

### 커밋 메시지 형식

```
<타입>: <한 줄 요약>

[선택] 상세 설명
```

| 타입 | 사용 시점 |
|------|----------|
| `feat` | 새 기능 |
| `fix` | 버그 수정 |
| `docs` | 문서 변경 |
| `refactor` | 리팩터링 |
| `test` | 테스트 추가/수정 |
| `chore` | 빌드·설정 변경 |

---

## 코드 스타일

- C++20 기준으로 작성합니다.
- `clang-format` 설정을 따릅니다.
- 함수·클래스 이름은 `snake_case`를 사용합니다.
- 공개 API는 주석으로 설명합니다.

---

## 커뮤니티 (향후 예정)

| 채널 | 내용 |
|------|------|
| Discord | 실시간 질문·토론 |
| Playground | 브라우저에서 drv 실행 |
| Benchmark Dashboard | 성능 추적 |
| 교육 문서 사이트 | 한국어 튜토리얼 웹사이트 |

로드맵 전체는 [ROADMAP.md](ROADMAP.md)를 참고하세요.

---
