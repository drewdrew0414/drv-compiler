# Security Policy

> Korean version: [docs/ko/SECURITY.md](docs/ko/SECURITY.md)

---

## Supported Versions

dri is in early development (`v0.x`). Security fixes are applied to the latest release only.

| Version | Supported |
|---------|-----------|
| `main` branch | ✅ |
| Previous tags | ❌ |

---

## Reporting a Vulnerability

**Do not open a public issue for security vulnerabilities.**

### Contact

**Email**: `drew0414drew@gmail.com`  
**Subject**: `[SECURITY] dri — <brief summary>`

### What to Include

1. Vulnerability type (e.g. buffer overflow, path traversal, injection)
2. Affected component (Lexer, Parser, codegen, runtime, CLI)
3. Minimal reproducible example — `.dri` source or CLI command
4. Expected impact

---

## Response Timeline

| Step | Target |
|------|--------|
| Acknowledgment | Within 3 days |
| Initial assessment | Within 7 days |
| Patch | Based on severity |
| Public disclosure | After patch ships |

---

## Security Model

### Compiler Safety

The dri compiler accepts trusted `.dri` source files. Issues triggered by malicious input are treated as security vulnerabilities.

Key areas:

- **Parser robustness** — malformed input must not crash or loop the compiler.
- **Codegen correctness** — generated C++ must not introduce unintended behavior.
- **`use` path safety** — module imports must not access files outside the project.
- **`sys.exec` injection** — passing unsanitized user input to `sys.exec()` is a risk; always validate first.

### Generated Code Safety

dri's ownership model provides compile-time memory safety guarantees:

| Mechanism | Guarantee |
|-----------|-----------|
| `Own<T>` + explicit `move` | No double-free, no use-after-move |
| `mut Borrow<T>` exclusivity | No data races in `parallel for` |
| `@region` escape analysis | No use-after-free for arena-allocated data |
| Array bounds checking | Enabled in `--debug` builds |

### Known Limitations

| API | Risk | Mitigation |
|-----|------|-----------|
| `sys.exec(cmd)` | Shell injection if user input passed directly | Validate/sanitize before use |
| `io.read_file(path)` | Path traversal if user controls path | Validate path before use |
| `io.mmap_read` inside `@region` | Use-after-free if reference escapes block | Compiler escape analysis blocks this at compile time |
| `--release` build | Disables array bounds checking | Use `--debug` when handling untrusted input |

---

## Security-Relevant Annotations

dri effect annotations make security-sensitive code explicit, aiding review:

```dri
@threadsafe
void safe_increment() { counter += 1; };

@io
String read_config(String path) { return io.read_file(path); };

@alloc
double[] make_buffer(int n) { return new double[n]; };
```

---

## Acknowledgments

Responsible disclosure is appreciated. With permission, reporters will be credited in release notes.

---
