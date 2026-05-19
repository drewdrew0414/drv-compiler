# Introduction to the dri Language
### Learn like Python. Scale like C++.

---

## Language Characteristics

dri is a language focused on the following goals.

| Goal | Reference Language | Description |
|------|---------|------|
| High productivity | Python | Intuitive and concise syntax |
| Strong safety | Rust | Ownership/borrow model + type safety |
| Top performance | C++ | Zero-cost abstractions, C++17 transpilation |
| Numerical computing | Julia | Built-in SIMD, tensor operations, automatic differentiation |

---

## Quick Start — Hello World

```dri
# hello.dri
print("Hello, dri!");

int x = 10;
double pi = 3.14159;
print("x =", x, "pi =", pi);

for (i in 0..5) {
    print(i);
};
```

Compile and run:

```bash
dri hello.dri --exe hello
./hello
```

Output:
```
Hello, dri!
x = 10 pi = 3.14159
0
1
2
3
4
```

---

## How It Works

dri source code is **transpiled** into C++17 code and then built by a C++ compiler.

```
hello.dri  →  [dri compiler]  →  hello.cpp  →  [clang/g++]  →  hello
```

- SIMD and parallel features leverage the optimization pipeline of clang/g++ directly.
- The `#line` directive ensures that error locations point back to the original `.dri` file.

---

## CLI Options

```
dri <input.dri> [options]
```

| Option | Description |
|------|------|
| `--exe <output>` | Generate an executable |
| `--cpp <output.cpp>` | Save only the C++ transpilation result |
| `--check` | Run syntax/type checking only, without compiling |
| `--debug` | Debug build (`-O0`, with bounds checking enabled) |
| `--release` | Release build (`-O3`, `-march=native`, LTO) |
| `--opt <0~3>` | Manually specify optimization level |
| `--native` | Enable `-march=native` |
| `--lto` | Enable Link-Time Optimization |
| `--trace <file>` | Generate a parallel-loop trace file (Chrome DevTools format) |
| `-D<flag>` | Define a compile-time flag (used in static_if conditions) |
| `--target <triple>` | Cross-compilation target (`x86_64-linux-gnu`, `aarch64-linux-gnu`, etc.) |
| `--sysroot <path>` | Path to the target OS's system headers/libraries |
| `--cross-cxx <compiler>` | Explicitly specify the cross-compiler binary |
| `--source-map <file>` | Generate a JSON source map (C++ line → dri line) |
| `--incremental` | Recompile only changed files |
| `--no-analyze` | Disable static analysis |

---

## PowerShell / CMD Command Differences

The dri command itself is identical across shells. However, the **build-then-run** (`&&`) syntax differs by shell.

### Compile only (identical on every shell)

```
dri main.dri --exe main.exe
dri main.dri --check
dri main.dri --cpp out.cpp
```

### Compile and run immediately

| Shell | Command |
|----|--------|
| **PowerShell 5.1** (default) | `& "dri.exe" main.dri --exe main.exe; if ($LASTEXITCODE -eq 0) { & ".\main.exe" }` |
| **PowerShell 7+** (pwsh) | `& "dri.exe" main.dri --exe main.exe && & ".\main.exe"` |
| **CMD** | `dri.exe main.dri --exe main.exe && main.exe` |
| **Bash / WSL** | `dri main.dri --exe main && ./main` |

> PowerShell 5.1 does not support the `&&` operator (it was added in PowerShell 7).
> The VSCode extension's **dri: Run File** button auto-detects the active terminal type and uses the correct syntax.

### Checking environment variables

| Item | PowerShell | CMD |
|------|------------|-----|
| Check PATH | `$env:PATH -split ';'` | `echo %PATH%` |
| Check installation path | `where.exe dri` | `where dri` |
| Check dri version | `dri --version` | `dri --version` |

```bash
# Basic usage
dri main.dri --exe main --release
dri main.dri --cpp out.cpp --check
dri main.dri --exe main --debug
dri main.dri --exe main --opt 2 --native --lto
dri main.dri --exe main --release --trace trace.json
```

---

## Language Rules at a Glance

| Rule | Content |
|------|------|
| Statement terminator | Every statement ends with `;` |
| Blocks | `{ }` braces are required |
| Comments | `#` for single-line, `!# ... ##` for multi-line |
| File extension | `.dri` |
| Entry point | Written at the top level of the file (no separate `main` function required) |

```dri
!# Multi-line comments
   are written like this.
##

# Single-line comment
int x = 42;   # Inline comment
print(x);
```

> For variables, types, and I/O basics, see [[02_basics.md](02_basics.md)].

---

## Cross-Compilation

The dri compiler can build binaries for platforms other than the current OS using the `--target` flag.

### Supported targets

| Target triple | Target environment | Description |
|------------|---------|------|
| `x86_64-linux-gnu` | Linux x86_64 | HPC servers, AWS/GCP EC2 |
| `aarch64-linux-gnu` | ARM64 Linux | AWS Graviton, Raspberry Pi 5 |
| `x86_64-w64-mingw32` | Windows 64-bit | Windows cross-build (Linux→Win) |
| `riscv64-linux-gnu` | RISC-V Linux | Embedded Linux boards |
| `wasm32-unknown-emscripten` | WebAssembly | Runs in the browser |

### Usage examples

```bash
# Build a Linux x86_64 AVX-512 binary from a Windows dev PC
dri main.dri --exe main_linux --target x86_64-linux-gnu --release

# Build for an ARM64 server (AWS Graviton)
dri main.dri --exe main_arm64 --target aarch64-linux-gnu --release

# Specify a sysroot (path to cross headers/libraries)
dri main.dri --exe out --target aarch64-linux-gnu --sysroot /opt/aarch64-sysroot

# Specify an explicit cross-compiler
dri main.dri --exe out --cross-cxx aarch64-linux-gnu-g++
```

### Automatic compiler detection order

1. If `clang++` is available, use the `--target=<triple>` flag (most flexible)
2. Otherwise, search for a `<triple>-g++` binary (e.g. `aarch64-linux-gnu-g++`)
3. If neither is found, emit an error along with installation instructions

```bash
# Install the ARM64 cross-compiler on Ubuntu
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

---

## Error Backtracing (Source Mapping)

The dri compiler automatically inserts `#line` directives into the generated C++ code. When the C++ compiler or debugger reports an error, it shows the exact line number in the `.dri` file.

```bash
# Default: #line directives are inserted automatically (always on)
dri main.dri --exe main

# Example error output
main.dri:12:5: error: 'undefined_var' was not declared in this scope
```

### Generating a JSON source map

```bash
dri main.dri --cpp out.cpp --source-map out.map.json
```

`out.map.json` format:
```json
{
  "version": 3,
  "sourceFile": "main.dri",
  "mappings": [
    {"cppLine": 45, "driFile": "main.dri", "driLine": 12},
    {"cppLine": 52, "driFile": "main.dri", "driLine": 15}
  ]
}
```

### GDB debugging

Thanks to `#line` directives, GDB also tracks `.dri` file locations:

```bash
dri main.dri --exe main --debug --source-map main.map.json
gdb ./main
(gdb) run
# On segfault: shows main.dri:23
```
