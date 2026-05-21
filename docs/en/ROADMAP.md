# dri Roadmap

The development direction for the dri compiler and language ecosystem.  
*Last updated: 2026-05-21 — v0.1.x safety patches + full v0.2 + partial v0.4 applied.*

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
| Runtime safety (bounds check, null deref, empty pop, negative fill) | ✅ Added in v0.1.1 |
| Example 08 — Compiler reordering & pipeline optimization | ✅ Added |
| Example 09 — Roofline model hardware benchmark | ✅ Added |

> **CLI options at a glance**: `--exe / --cpp / --check / --debug / --release /
> --opt N / --native / --lto / --trace / --incremental / --cache-dir /
> --no-analyze / --target / --sysroot / --cross-cxx / --source-map /
> --strict / --link <libs> / -D<FLAG>`

---

## v0.2 — Toolchain Hardening (next milestone)

**Goal**: take the shipped transpiler and polish it for real-world use.

### Compiler quality
- [x] Unified error message format — every stage uses `file:line:col: kind: msg`
- [x] Multilingual diagnostics (`--lang ko|en|ja`)
- [ ] `--source-map` validation tooling
- [x] Regression test suite (`tests/cases/*.dri` + `tests/run_tests.sh`, 23 tests)
- [x] CI matrix (`.github/workflows/ci.yml` — Linux/macOS × clang/g++)

### Build system
- [x] `dri build` multi-file project manifest (`dri.drpm`) — `name/version/main/output/link/search_dirs`
- [x] Linking against system static libraries (`--link math,m,pthread`)
- [x] Auto-sign installers (Windows `sign.ps1` with EV cert detection)
- [ ] Push the artifact from `installer/linux/build-installer.sh` to the R2 registry automatically

### Safety
- [x] `--strict` mode (warnings promoted to errors)
- [x] Integer overflow checks (`@checked_arith` annotation)
- [x] Module import cycle detection (DFS-based, prints the full import chain)

---

## v0.3 — Type System Strengthening

**Goal**: turn generics and ownership from pass-through mappings into a system
that is *actually checked*.

- [ ] Real generic functions / classes (today they are forwarded as C++ templates)
- [ ] Trait-bound validation (`T implements Printable` → require an impl exists)
- [x] Ownership flow analysis — use-after-move compile error (move-after-use detection)
- [ ] `Borrow<T>` lifetime checking (escape analysis)
- [ ] `dim` dimensional unit type conflict detection (`@units_check`)
- [ ] Reflection (`reflect.type_of`, `reflect.fields`)
- [x] Warnings for unhandled `Option<T>` / `Result<T>` — discarded return values

---

## v0.4 — HPC Runtime Expansion

**Goal**: move beyond plain OpenMP into a runtime tuned for real HPC workloads.

- [x] AVX-512 auto-dispatch (`@avx512` → runtime dispatcher + avx512f clone)
- [ ] Work-stealing thread pool (for `parallel for of` collections)
- [x] `tensor<N, T>` fixed-dimension SIMD-friendly array — mapped to `std::array<T,N>`
- [x] `simd.fmadd/fmsub/fnmadd`, `math.invsqrt`, `bits.popcount/clz/ctz/bswap/rotl/rotr/log2` intrinsic mapping
- [x] `mem.prefetch/fence/zero`, `sys.affinity/time_ms/cpu_count`, `sys.sync.*` memory annotations
- [ ] Non-blocking I/O (`async io.read_file` → `io_uring` / `kqueue` / `IOCP`)
- [x] `--trace` output in Chrome `tracing` format (μs units, numeric `ts`/`dur`, includes `@bench` functions)

---

## v0.5 — Compile-time Metaprogramming

**Goal**: let users express logic that runs at compile time.

- [x] `diff.forward` / `diff.numerical` / `diff.hessian` auto-differentiation helpers
- [x] `@bench` measurements attached to the build artifact as JSON (`--bench-json <file>`)
- [x] `@specialize<T=float, double>` per-type SIMD specialization (explicit template instantiation)
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
  - [x] `dri init` — project scaffold (dri.drpm + src/ + tests/ directories)
  - [ ] `drvpm install` — SemVer + lockfile dependency install
  - [ ] `drvpm publish` — publish to the R2 registry
  - [x] `dri build` / `dri test` — task runner (build from manifest, run tests/)
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
