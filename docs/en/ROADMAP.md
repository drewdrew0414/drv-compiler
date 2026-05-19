# dri Roadmap

The development direction for the dri compiler and language ecosystem.  
*Last updated: 2026-05-19 — anchored at the v0.1.0 release.*

---

## Current State (v0.1.0 — Transpiler MVP shipped)

The full C++20 transpiler pipeline (lexer → parser → static analysis →
code generation → backend invocation) is wired up end-to-end, and dri
source compiles to a real executable.

| Area | Status |
|------|--------|
| Core syntax (variables, types, I/O) | ✅ Implemented |
| Control flow (`if`, `for`, `while`, `switch`, `match`) | ✅ Implemented |
| Functions, classes, inheritance, traits, `impl` | ✅ Implemented |
| Lambdas, `|>` pipes, higher-order functions | ✅ Implemented |
| Collections (`array`, `list`, `Map`, basic `tensor`) | ✅ Implemented |
| Error handling (`try/catch`, `Option`, `Result`, `match`) | ✅ Implemented |
| Memory model (`Own`, `Ref`, `Borrow`, `move`, `@region`) | ✅ Implemented |
| SIMD loops (`simd for`) + reductions | ✅ Implemented (OpenMP `simd`) |
| Parallel loops (`parallel for`, `parallel simd for`) | ✅ Implemented (OpenMP) |
| `async / await / spawn` | ✅ Implemented (`std::async` / threads) |
| Annotations (`@inline`, `@pure`, `@noalias`, `@threadsafe`, `@bench`, `@trace`, `@specialize`, `@fast_math`, `@stack/@heap`, `@warrant/@rebuttal/@defeats`) | ✅ Implemented |
| `compile_eval` / `static_if` | ✅ Implemented |
| Built-in functions (str, lst, map, math, io, sys, diff.*) | ✅ Implemented |
| Module system (`module` / `use`) | ✅ First-stage module loading, dedup of duplicates |
| `#line` directives + source maps (JSON) | ✅ Implemented |
| Static analyzer (thread-safety, aliasing, `@packed` inheritance, `atomic<String>`) | ✅ Implemented |
| Incremental build cache (CRC32 + mtime) | ✅ Implemented |
| Cross-compilation (`--target`, `--sysroot`, `--cross-cxx`) | ✅ Implemented |
| Backend (`clang++` / `g++`, `-fopenmp`, `-flto`, `-march=native`) | ✅ Implemented |
| Windows / macOS / Linux installers | ✅ First release (PS1 + WinForms + Bash) |

> **CLI options at a glance**: `--exe / --cpp / --check / --debug / --release /
> --opt N / --native / --lto / --trace / --incremental / --cache-dir /
> --no-analyze / --target / --sysroot / --cross-cxx / --source-map / -D<FLAG>`

---

## v0.2 — Toolchain Hardening (next milestone)

**Goal**: take the shipped transpiler and polish it for real-world use.

### Compiler quality
- [ ] Unified error message format — every stage uses `file:line:col: kind: msg`
- [ ] Multilingual diagnostics (`--lang ko|en|ja`)
- [ ] `--source-map` validation tooling
- [ ] Regression test suite (`tests/cases/*.dri` + expected output diffs)
- [ ] CI matrix (Linux / macOS / Windows × clang / g++)

### Build system
- [ ] `dri build` multi-file project manifest (`dri.toml`)
- [ ] Linking against system static libraries (`--link math,m,pthread`)
- [ ] Auto-sign installers (Apple notarization, Windows Authenticode)
- [ ] Push the artifact from `installer/linux/build-installer.sh` to the R2 registry automatically

### Safety
- [ ] `--strict` mode (warnings promoted to errors)
- [ ] Integer overflow checks (`@checked_arith` annotation)
- [ ] Module import cycle detection

---

## v0.3 — Type System Strengthening

**Goal**: turn generics and ownership from pass-through mappings into a system
that is *actually checked*.

- [ ] Real generic functions / classes (today they are forwarded as C++ templates)
- [ ] Trait-bound validation (`T implements Printable` → require an impl exists)
- [ ] Ownership flow analysis (using a value after `move` becomes a compile error)
- [ ] `Borrow<T>` lifetime checking (escape analysis)
- [ ] `dim` dimensional unit type conflict detection (`@units_check`)
- [ ] Reflection (`reflect.type_of`, `reflect.fields`)
- [ ] Warnings for unhandled `Option<T>` / `Result<T>`

---

## v0.4 — HPC Runtime Expansion

**Goal**: move beyond plain OpenMP into a runtime tuned for real HPC workloads.

- [ ] AVX-512 auto-dispatch (`__attribute__((target("avx512f")))` cloned functions)
- [ ] Work-stealing thread pool (for `parallel for of` collections)
- [ ] `tensor<N, T>` fixed-rank SIMD-optimized arrays
- [ ] `simd.fmadd`, `math.invsqrt`, `bits.popcount` intrinsic mapping
- [ ] `mem.prefetch`, `sys.affinity`, `sys.numa` memory annotations
- [ ] Non-blocking I/O (`async io.read_file` → `io_uring` / `kqueue` / `IOCP`)
- [ ] `--trace` output in Chrome `tracing` format (currently a stub)

---

## v0.5 — Compile-time Metaprogramming

**Goal**: let users express logic that runs at compile time.

- [ ] `diff.forward` / `diff.numerical` / `diff.hessian` auto-differentiation codegen
- [ ] `@bench` measurements attached to the build artifact as JSON
- [ ] `@specialize<T=float, double>` per-type SIMD specialization
- [ ] Standard library of compile-time helpers usable inside `static_if`
- [ ] `@warrant / @rebuttal / @defeats` metadata embedded as an ELF / Mach-O section
- [ ] `extern "FFI"` safe interop with modern languages (Rust, Zig → auto `Own/Ref` mapping)
- [ ] `@unsafe_legacy extern "C"` legacy defense layer (auto-injects signal handlers)

---

## v0.6 — Library Ecosystem

**Goal**: ship the dri-pandas, dri-toulmin, dri-data, dri-plot core libraries.

- [ ] **dri-pandas** — DataFrame pipeline
  - [ ] FFI wrapping around Apache Arrow / Rust Polars
  - [ ] `.query()`, `.assign()`, `.groupby()`, `.mean()` method chaining
  - [ ] `df.to_tensor()` tensor conversion
- [ ] **dri-toulmin** — argument rule engine ([reference](https://github.com/park-jun-woo/toulmin))
  - [ ] Acceptability: $Acc(a) = \frac{w(a)}{1 + \sum Acc(attackers)}$
  - [ ] `toulmin.load_rules()` to load binary metadata
  - [ ] `toulmin.build_audit()` audit trail tree
- [ ] **dri-data** — multi-format I/O
  - [ ] High-speed CSV, JSON, XML serialization
  - [ ] DataFrame → PNG export
  - [ ] Argument tree → PNG export
- [ ] **dri-plot** — data visualization
  - [ ] line / bar / scatter / heatmap chart engine
  - [ ] `plot.dashboard()` composable dashboards
  - [ ] GUI window (`show()`) and HTML/PNG export

---

## v1.0 — Package Manager + Self-hosting

**Goal**: a usable dri ecosystem, including a compiler written in dri.

- [ ] `drvpm` package manager
  - [ ] `drvpm init` — project bootstrap
  - [ ] `drvpm install` — SemVer + lockfile dependency install
  - [ ] `drvpm publish` — publish to the R2 registry
  - [ ] `drvpm build/test/run` — task runner
- [ ] Standard library packages (`math`, `tensor`, `http`, `json`)
- [ ] LSP server — IDE autocomplete / diagnostics / hover (`dri-lsp`)
- [ ] Playground — run dri in the browser (WASM compiler + WASI runtime)
- [ ] Self-hosting (a first-pass dri compiler written in dri)

---

## Long-term (v1.1+)

| Plan | Description |
|------|-------------|
| LLVM backend | Emit LLVM IR directly instead of transpiling to C++ |
| Incremental modules | Cache `*.dri.bc` so only changed modules recompile |
| WASM target | WebAssembly output (shared with the Playground backend) |
| GPU kernels | CUDA / SPIR-V / Metal backends (`@gpu parallel for`) |
| Interactive REPL | Ad-hoc dri execution (LSP-driven) |
| Interpreter mode | Scripting-style runs (`dri --run script.dri`) |
| Debugger integration | DWARF line mappings + custom gdb/lldb pretty-printers |

---

## Contributing

If you want to contribute to a specific roadmap item, open an issue first.  
See [CONTRIBUTING.md](CONTRIBUTING.md) for the contribution guide.

---
