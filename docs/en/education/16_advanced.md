# Advanced Features
> dri's advanced language features: module system, compile-time branching, auto-differentiation, build profiles, package manager.

---

## Module System — `module` / `use`

dri supports file-level modules. Each `.dri` file is an independent compilation unit, and only symbols declared `public` are exposed externally.

### Module declaration — `module`

```dri
# math_utils.dri
module math_utils;

public double fancy_sqrt(double x) {
    return sqrt(x * 2.0);
};

public double lerp(double a, double b, double t) {
    return a + (b - a) * t;
};

private double internal_helper(double x) {
    return x * x;   # Not accessible from outside
};
```

### Using modules — `use`

```dri
# main.dri
use math_utils;                    # Import the entire module
use math_utils::fancy_sqrt;        # Import only specific symbols

double result = fancy_sqrt(9.0);
print(result);   # 6.0

double v = math_utils::lerp(0.0, 100.0, 0.5);
print(v);        # 50.0
```

### Compiler behavior

```
math_utils.dri  →  [independent compile]  →  math_utils.o
main.dri        →  [independent compile]  →  main.o
                →  [link]                 →  executable
```

- Only changed modules are recompiled (incremental builds)
- Symbol scope is isolated — no name conflicts
- Circular dependencies are detected at compile-time

---

## static_if — Compile-time conditional branching

`static_if` is **not a preprocessor macro.** Like C++'s `if constexpr`, it is evaluated at the AST level.
False branches are **parsed but skipped during code generation** — this is not a textual inlining mechanism.

### Build flag conditions (→ C++ `if constexpr` + build flags)

```dri
static_if (RELEASE_BUILD) {
    print("This is a release build");
} else {
    print("This is a debug build");
};
```

If the `-DRELEASE_BUILD` flag is given at compile time, only the then-branch is code-generated.

> **Relation to the module system**: Because `static_if` is not textual preprocessing,
> it also applies to **precompiled binary modules** distributed via `drvpm`.
> Libraries that use `static_if` do not need to publish their full source code.
> Flag values are finalized at link time.

### Expression conditions (→ C++ `if constexpr`)

```dri
static_if (1 == 1) {
    print("always true");
};
```

### `static_if` vs runtime `if`

| | `static_if` | `if` |
|--|-------------|------|
| Evaluation time | Compile-time | Runtime |
| False branch | No code generated | Code generated |
| Condition kind | Constants / build flags | Any expression |
| Use case | Platform / build branching | General conditional branching |

---

## @bench — Execution time measurement

Annotating a function with `@bench` automatically measures its execution time and prints the result to stderr.

```dri
@bench
void heavy_computation() {
    double sum = 0.0;
    for (int i = 0; i < 1000000; i++) {
        sum = sum + math.sqrt(i as double);
    };
};

heavy_computation();
# stderr output: [bench] heavy_computation: 12.345 ms
```

- Implemented via RAII: the timer starts on function entry and the measurement is taken automatically on return
- Accurate even when there are multiple `return` statements

---

## @specialize — Per-type SIMD specialization

Generates optimized code paths for specific types. Functions marked with `@specialize` are tagged `DRV_HOT` and get higher priority during auto-vectorization.

```dri
@specialize
double dot_product(tensor a, tensor b) {
    double result = 0.0;
    for (int i = 0; i < length(a); i++) {
        result = result + a[i] * b[i];
    };
    return result;
};
```

---

## Auto-differentiation (Auto-diff)

dri supports auto-differentiation at the language level.

### diff.forward — Forward-mode auto-differentiation

```dri
# f(x) = x^2 + 2x + 1 → f'(x) = 2x + 2
double f(double x) {
    return x * x + 2.0 * x + 1.0;
};

# At x = 3.0, f'(3) = 8.0
var df = diff.forward(f, 3.0);
print(df);  # 8.0
```

### diff.numerical — Numerical differentiation (central difference)

```dri
double g(double x) {
    return math.sin(x);
};

var dg = diff.numerical(g, math.pi() / 4.0);
print(dg);  # cos(π/4) ≈ 0.7071...
```

### diff.hessian — Second derivative

```dri
double h(double x) {
    return x * x * x;  # h''(x) = 6x
};

var d2h = diff.hessian(h, 2.0);
print(d2h);  # 12.0
```

---

## Build Profiles

### Debug mode

```bash
dri main.dri --exe main.exe --debug
```

- Optimization level: `-O0`
- Fast-math disabled
- LTO disabled
- Runtime checks enabled (array bounds checking, etc.)

### Release mode

```bash
dri main.dri --exe main.exe --release
```

- Optimization level: `-O3`
- `-march=native` (optimization tuned for the current CPU)
- LTO (Link-Time Optimization) enabled
- Fast-math enabled

### Manual configuration

```bash
dri main.dri --exe main.exe --opt 2 --native --lto
```

---

## Trace Export — Parallel loop visualization

Records thread occupancy of `parallel` loops in Chrome DevTools format.

```bash
dri main.dri --exe main.exe --trace trace.json
```

```dri
parallel for (x of data) {
    process(x);
};
```

Open the generated `trace.json` in `chrome://tracing` or Perfetto to inspect per-thread execution timelines.

---

## Package manager (drvpm)

Provided as a separate executable, `drvpm`. It supports Gradle-style dependency management, incremental builds, and integration with an R2 registry.

### Initializing a new project

```bash
drvpm init my-project
cd my-project
```

Generated structure:
```
my-project/
  src/main.dri       # Entry point
  tests/main.dri     # Test suite
  drvpm.drii         # Project metadata
  build/             # Build artifacts (auto-generated)
```

### drvpm.drii format (Gradle-style dependency management)

`drvpm.drii` plays the same role as Gradle's `build.gradle`.
It supports dependency resolution, platform-conditional dependencies, and development-only dependencies.

```ini
[project]
name        = "my-project"
version     = "0.1.0"
author      = "drewdrew0414"
description = "My dri project"
license     = "MIT"
dri_min     = "0.6.0"           # Minimum dri compiler version
registry    = "https://drvpm-registry.cloud"

# ── Runtime dependencies (included in distribution) ───────────
[dependencies]
math          = "^1.0.0"
tensor        = "~2.3.0"
dri-pandas    = "^0.2.0"
dri-toulmin   = "^0.1.0"
dri-data      = "^0.1.0"
dri-plot      = "^0.1.0"

# ── Dev-only dependencies (excluded from distribution, same as Gradle devDependencies) ─
[dev_dependencies]
dri-test      = "^0.1.0"        # Test framework
dri-bench     = "^0.1.0"        # Benchmark framework
dri-lint      = "^0.1.0"        # Static analyzer

# ── Platform-conditional dependencies (same as Gradle platform() / configurations) ─
[dependencies.platform.linux]
cuda-dri      = "^12.0.0"       # CUDA integration only on Linux

[dependencies.platform.windows]
directml-dri  = "^1.0.0"        # DirectML integration only on Windows

[dependencies.platform.macos]
metal-dri     = "^3.0.0"        # Metal integration only on macOS

# ── Feature-flag conditional dependencies ─────────────────────
[dependencies.feature.gpu]
cuda-dri      = "^12.0.0"
opencl-dri    = "^3.0.0"

[dependencies.feature.hpc]
mpi-dri       = "^4.0.0"
blas-dri      = "^3.10.0"

# ── Repository configuration (same as Gradle repositories) ────
[repositories]
central   = "https://drvpm-registry.cloud"
local     = "./local_packages"
mirror    = "https://mirror.company.internal/drvpm"

# ── Dependency resolution strategy ────────────────────────────
[resolution]
strategy    = "newest-compatible"   # newest-compatible | strict | lowest
fail_on_conflict = true

# ── Task definitions (same as Gradle tasks) ───────────────────
[tasks]
build   = "dri src/main.dri --exe build/my-project --release"
test    = "dri tests/main.dri --exe build/test && build/test"
lint    = "dri src/main.dri --check"
bench   = "dri bench/main.dri --exe build/bench --release && build/bench"
docs    = "dridoc src/ --out docs/api/"
deploy  = "drvpm publish"
```

### Dependency inheritance — `extend` (same as Gradle BOM)

Inherit shared configuration from a parent project.

```ini
[project]
name   = "my-service"
extend = "company-base-drii@^1.0.0"   # Inherit parent settings

[dependencies]
# Versions of dri-pandas, dri-data, etc. from the parent are inherited automatically
# Only declare the ones you need to override
dri-pandas = "^0.3.0"   # Override the parent version
```

### Version catalog — `[versions]`

Manage team-wide versions in one place (same as Gradle Version Catalog).

```ini
[versions]
dri-pandas-ver  = "0.2.3"
dri-plot-ver    = "0.1.5"
arrow-core-ver  = "14.0.0"

[dependencies]
dri-pandas = { version = "$dri-pandas-ver", features = ["arrow", "parquet"] }
dri-plot   = { version = "$dri-plot-ver",   features = ["webgl"] }
```

### Main commands

```bash
drvpm install              # Install dependencies (generates drvpm.lock)
drvpm install math@^1.0.0  # Add a specific package

drvpm build                # Incremental build
drvpm build --debug
drvpm build --release

drvpm run                  # Run
drvpm test                 # Test

drvpm search math          # Search packages
drvpm list                 # List installed packages
drvpm clean                # Clean build artifacts
```

### Lock file (deterministic builds)

`drvpm install` automatically generates `drvpm.lock`:

```ini
[meta]
generated = "2026-05-16T10:00:00Z"

[math]
version = "1.2.3"
hash    = "sha256:abc123..."
url     = "https://drvpm-registry.cloud/packages/math/1.2.3.tar.gz"
```

When teammates run `drvpm install`, the exact same versions are installed based on the lock file.

### SemVer constraint notation

| Notation | Meaning |
|----------|---------|
| `^1.2.0` | `>=1.2.0 <2.0.0` (major fixed) |
| `~1.2.0` | `>=1.2.0 <1.3.0` (minor fixed) |
| `>=1.0.0` | 1.0.0 or higher |
| `1.2.3` | Exact version |
| `*` | Any version |

### Package publishing (R2)

```bash
drvpm login ACCESS_KEY_ID SECRET_ACCESS_KEY \
  https://ACCOUNT.r2.cloudflarestorage.com my-bucket

drvpm publish
```

### Custom tasks

```bash
drvpm task lint
drvpm task deploy
```

---

## Source Map

Translates errors in the C++ code generated by the compiler back to their original dri source positions. Through `#line` directives, the C++ compiler reports dri file locations directly.

---

## @units_check — Physical unit checking

Used together with `dim` declarations to verify physical unit consistency at compile time.

```dri
dim Meter = "m";
dim Second = "s";
dim Kg = "kg";

Meter distance = Meter(10.0);
Second time = Second(5.0);

# Only values with the same unit can be added
Meter total = distance + Meter(3.0);  # OK
# Meter wrong = distance + time;       # Compile error!

# Division: produces a different unit
double speed = distance / time;  # m/s (auto-converted to double)
```

The C++ type system rejects addition/subtraction of values with different unit tags at compile time.

> See [[02_basics.md](02_basics.md)] for `dim` dimensional unit type declarations.

---

## extern "C" — C library integration

You can call existing HPC assets (BLAS, LAPACK, CUDA, etc.) directly from dri.
When you declare a C function inside an `extern "C"` block, the dri compiler links it via the C ABI without name mangling. Array arguments are automatically converted to raw pointers (`&arr[0]`).

```
extern "C" {
    return_type function_name(parameters);
};
```

```dri
# BLAS dgemm declaration (matrix multiplication)
extern "C" {
    void cblas_dgemm(int Order, int TransA, int TransB,
                     int M, int N, int K,
                     double alpha, double[] A, int lda,
                                   double[] B, int ldb,
                     double beta,  double[] C, int ldc);
};

double[] A = new double[1024];
double[] B = new double[1024];
mut double[] C = new double[1024];

# At call time, arrays are auto-converted to raw pointers like &A[0]
cblas_dgemm(101, 111, 111, 32, 32, 32,
            1.0, A, 32, B, 32,
            0.0, C, 32);
```

Declaring multiple functions:

```dri
extern "C" {
    void cblas_daxpy(int n, double alpha, double[] x, int incx,
                     double[] y, int incy);
    double cblas_ddot(int n, double[] x, int incx, double[] y, int incy);
    void cblas_dscal(int n, double alpha, double[] x, int incx);
};
```

| dri type | C pointer conversion |
|----------|---------------------|
| `double[]` | `double*` (→ `&arr[0]`) |
| `int[]` | `int*` |
| `mut double[]` | `double*` (writable) |
| `Borrow<double[]>` | `const double*` |

> **The `mut` keyword**: If a C function modifies the array, declare it as `mut double[]`.
> Without `mut`, the compiler passes a `const` pointer, making writes from the C side undefined behavior.

---

## extern "FFI" — Safe integration with modern languages

Safely maps functions and structs from **languages that provide a C-compatible ABI with metadata** (Rust, Zig, etc.) into dri's type system.
The compiler analyzes the type information and automatically generates `Own<T>` and `Ref<T>` bindings.

```
extern "FFI" {
    use language::path::Type;
    fn function_name(parameters) -> return_type;
};
```

```dri
extern "FFI" {
    # Rust library integration
    use rust::polars::DataFrame;
    use rust::polars::Series;

    fn polars_read_csv(path: String) -> Own<DataFrame>;
    fn polars_filter(df: Ref<DataFrame>, expr: String) -> Own<DataFrame>;
    fn polars_mean(series: Ref<Series>) -> double;
};

Own<DataFrame> df = polars_read_csv("data.csv");
Own<DataFrame> filtered = polars_filter(df, "age > 30");
print(polars_mean(filtered.get_column("salary")));
```

### Automatic Own<T> / Ref<T> mapping rules

| Language side | dri mapping | Meaning |
|---------------|-------------|---------|
| Rust `Box<T>` / `T` (returned) | `Own<T>` | Ownership transfer |
| Rust `&T` | `Borrow<T>` | Immutable reference |
| Rust `&mut T` | `mut Borrow<T>` | Mutable reference |
| Rust `Arc<T>` | `Ref<T>` | Shared ownership |

---

## @unsafe_legacy extern "C" — Legacy defense layer

Old C and Fortran libraries can cause **entire-process crashes** such as SegFault or stack overflow.
Adding `@unsafe_legacy` causes the compiler to automatically inject **signal handlers and isolation wrappers** at each call site.
Even if the library crashes, the dri runtime catches the signal and returns the error as a `Result<T>`.

```dri
@unsafe_legacy extern "C" {
    # Fortran LAPACK routine (SegFault risk)
    void dgesv_(int[] N, int[] NRHS, double[] A, int[] LDA,
                int[] IPIV, double[] B, int[] LDB, int[] INFO);

    # Legacy image processing C library
    int legacy_encode(mut double[] buf, int size, String format);
};
```

The wrapper auto-generated by the compiler at call time:

```cpp
// Auto-generated C++ wrapper (conceptual)
Result<void> __dri_safe_dgesv_(...) {
    struct sigaction sa_segv, sa_fpe;
    // Register SIGSEGV, SIGFPE handlers
    // ...
    try {
        dgesv_(...);
        return Ok();
    } catch (...) {
        return Err("legacy call crashed: dgesv_");
    }
}
```

```dri
# From dri code, the result is safely received as a Result<>
Result<void> res = dgesv_(n, nrhs, A, lda, ipiv, B, ldb, info);
match res {
    Ok(_)  => { print("LAPACK succeeded"); };
    Err(e) => { print("Legacy crash:", e); };
};
```

### Comparison of extern variants

| Syntax | Target language | Safety | Type mapping |
|--------|-----------------|--------|--------------|
| `extern "C"` | C (trusted) | Developer's responsibility | Manual |
| `extern "FFI"` | Rust, Zig | Automatic compiler verification | Automatic (`Own`/`Ref`) |
| `@unsafe_legacy extern "C"` | C, Fortran (legacy) | Automatic signal isolation injection | Manual + wrapper |

---

## Static Analyzer

At compile time, the following 4 passes run automatically:

| Pass | Detection target |
|------|------------------|
| Thread safety | Shared variable writes inside `parallel for` without synchronization |
| Aliasing verification | Duplicate variable passed to `@noalias` function call |
| `@packed` inheritance | Simultaneous use of `@packed` + `extends` (compile error) |
| `atomic<String>` | Warning about limitation that only pointers are atomic |

All warnings can be disabled with `--no-analyze` (for faster release builds):

```bash
dri main.dri --exe app --release --no-analyze
```

---

## New CLI flags

| Flag | Description |
|------|-------------|
| `--incremental` | Recompile only changed files |
| `--cache-dir <path>` | Specify the incremental build cache directory |
| `--no-analyze` | Disable static analysis |

---

## Cross-compilation flag summary

| Flag | Description |
|------|-------------|
| `--target <triple>` | Set cross-compilation target |
| `--sysroot <path>` | Path to target OS system headers/libraries |
| `--cross-cxx <cc>` | Specify the cross-compiler binary |
| `--source-map <f>` | Generate a JSON source map file |

### Typical usage patterns

```bash
# HPC server deployment (Intel AVX-512 optimized)
dri compute.dri --exe compute_server \
    --target x86_64-linux-gnu \
    --release

# Edge device deployment (ARM64)
dri edge.dri --exe edge_binary \
    --target aarch64-linux-gnu \
    --sysroot /opt/arm-sysroot \
    --release

# Debug build + source map
dri main.dri --exe main_debug \
    --debug --source-map main.map.json
```
