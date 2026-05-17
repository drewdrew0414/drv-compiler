# Contributing to drv

drv is an open-source project targeting education and HPC systems programming.

> Korean guide: [docs/ko/CONTRIBUTING.md](docs/ko/CONTRIBUTING.md)

## Values

- **Simplicity** — easy-to-learn syntax
- **Performance** — C++ execution speed
- **Safety** — ownership/borrow model, type safety
- **Education-friendly** — clear error messages, intuitive concepts
- **Parallel accessibility** — `parallel for`, `simd for`, `async/await` without friction

---

## Getting Started

### Clone

```bash
git clone https://github.com/drewdrew0414/drv-compiler.git
cd drv-compiler
```

### Build

```bash
cmake -B build
cmake --build build
```

### Test

```bash
cd build && ctest --output-on-failure
```

---

## Project Structure

```
drv-compiler/
├── main.cpp              # Compiler entry point
├── compiler/
│   └── src/              # Lexer, Parser, AST, Codegen
├── docs/
│   └── ko/
│       ├── education/    # Korean education docs (01–16)
│       ├── CONTRIBUTING.md
│       ├── ROADMAP.md
│       └── SECURITY.md
├── CMakeLists.txt
└── LICENSE
```

---

## Areas to Contribute

### Compiler Core

| Area | Details |
|------|---------|
| Lexer | Tokenization, keywords, multiline comments |
| Parser | Grammar rules, AST construction |
| Type System | `var` inference, generics, trait bounds, `dim` unit types |
| Ownership Checker | `Own<T>` move rules, `mut Borrow<T>` exclusivity, `@region` escape analysis |
| Code Generation | C++17 transpiler, `#line` source maps |

### HPC Runtime

| Area | Details |
|------|---------|
| SIMD | AVX2/AVX-512/NEON auto-vectorization, `where`/`otherwise` masks |
| Parallel | Work-stealing thread pool, `reduction` syntax |
| Memory | `@region` arena, `@align(N)`, `@layout_soa` |
| Auto-diff | `diff.forward`, `diff.numerical`, `diff.hessian` |

### Language Features (Planned)

| Feature | Status |
|---------|--------|
| `diff.*` auto-differentiation | Designing |
| `@units_check` dimensional analysis | Designing |
| Package manager `drvpm` | Designing |
| LLVM backend | Future |

### Documentation

- Korean education docs (`docs/ko/education/`) — 16 files, full spec
- Example `.drv` programs

---

## Contribution Process

1. **Open an issue first** — describe what you plan to work on.
2. **Branch from `main`**:
   ```bash
   git checkout -b feat/my-feature
   ```
3. **Commit** with conventional format:
   ```bash
   git commit -m "feat: short description"
   ```
4. **Open a Pull Request** against `main`.

### Commit Message Format

```
<type>: <short summary (≤50 chars)>

[optional body — explain the why, not the what]
```

| Type | When |
|------|------|
| `feat` | New language feature or compiler capability |
| `fix` | Bug fix |
| `docs` | Documentation only |
| `refactor` | Code restructure, no behavior change |
| `test` | Tests |
| `chore` | Build config, tooling |

---

## Code Style

- C++20 standard.
- Follow `clang-format` config at repo root.
- `snake_case` for functions, classes, variables.
- Comment public APIs — one short line, explain the *why* not the *what*.

---

## Community (Planned)

| Channel | Purpose |
|---------|---------|
| Discord | Real-time discussion, questions |
| Playground | Run drv in the browser |
| Benchmark Dashboard | Track performance over commits |

Full roadmap: [ROADMAP.md](ROADMAP.md)

---
