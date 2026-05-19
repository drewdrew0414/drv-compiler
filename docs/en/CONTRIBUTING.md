# Contributing to dri

dri is an open-source project aiming to be both an educational language and an
HPC systems language.

What we care about:

- **Simplicity** — easy-to-learn syntax
- **Performance** — execution speed on par with C++
- **Safety** — ownership / borrow model, type safety
- **Education** — clear error messages, intuitive concepts
- **Approachable parallelism** — `parallel`, `simd for`, `async/await` made easy

---

## Getting Started

### Clone the repository

```bash
git clone https://github.com/drewdrew0414/dri-compiler.git
cd dri-compiler
```

### Build

```bash
cmake -B build
cmake --build build
```

### Test

```bash
cd build
ctest --output-on-failure
```

---

## Project Layout

```
dri-compiler/
├── main.cpp              # compiler entry point
├── compiler/
│   └── src/              # compiler sources (Lexer, Parser, AST, Codegen, Analyzer, Incremental)
├── docs/
│   ├── en/               # English docs
│   ├── ja/               # Japanese docs
│   └── ko/               # Korean docs (canonical source)
├── installer/
│   ├── linux/            # Linux/macOS installer + bundler
│   └── window/            # Windows installer (PS1 + WinForms)
├── CMakeLists.txt
└── LICENSE
```

---

## Areas to Contribute

### Compiler core

| Area | Description |
|------|-------------|
| Lexer | Tokenization, new keywords |
| Parser | Grammar rules, AST construction |
| AST | Node optimization, visitor patterns |
| Type system | Inference, generics, dimensional units |
| C++ codegen | Improve transpile output quality |

### HPC runtime

| Area | Description |
|------|-------------|
| SIMD | AVX2 / AVX-512 / NEON auto-vectorization |
| Parallel | Improve work-stealing thread pool |
| NUMA | Memory locality optimizations |
| tensor | Numeric ops acceleration |

### Language features

| Feature | Status |
|---------|--------|
| `diff.*` auto-differentiation | In design |
| `@units_check` dimensional types | In design |
| `drvpm` package manager | In design |
| LLVM backend | Future plan |

### Documentation

- Expand the Korean education docs (`docs/ko/education/`)
- Keep `docs/en/` and `docs/ja/` in sync with the Korean canonical source
- Write more example programs

---

## Contribution Flow

1. Open an issue first to share the planned work.
2. Branch from `main`:
   ```bash
   git checkout -b feat/my-feature
   ```
3. Commit your changes:
   ```bash
   git commit -m "feat: short description"
   ```
4. Open a Pull Request.

### Commit message format

```
<type>: <one-line summary>

[optional] details
```

| Type | When to use |
|------|-------------|
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation only |
| `refactor` | Refactoring with no functional change |
| `test` | Adding or updating tests |
| `chore` | Build / config changes |

---

## Code Style

- Target C++20.
- Follow the project's `clang-format` config.
- Use `snake_case` for function and class names.
- Document public APIs with brief comments.

---

## Community (planned)

| Channel | Purpose |
|---------|---------|
| Discord | Live Q&A and discussion |
| Playground | Run dri in the browser |
| Benchmark dashboard | Track performance over time |
| Education site | Hosted multilingual tutorial site |

See [ROADMAP.md](ROADMAP.md) for the full roadmap.

---
