# Security Policy

---

## Supported Versions

dri is in early development (`v0.x`).  
Security fixes apply only to the latest release.

| Version | Supported |
|---------|-----------|
| `main` branch | ✅ Yes |
| Older tags | ❌ No |

---

## Reporting a Vulnerability

If you discover a security vulnerability, **please report it privately
rather than opening a public issue**.

### How to report

**Email**: `drew0414drew@gmail.com`  
Subject line format: `[SECURITY] dri - <vulnerability summary>`

### What to include

1. Type of vulnerability (e.g. buffer overflow, path traversal, code injection)
2. Affected component (lexer, parser, codegen, …)
3. A minimal reproducible example (a `.dri` snippet or CLI invocation)
4. Estimated blast radius

---

## Triage Timeline

| Stage | Description | Target window |
|-------|-------------|---------------|
| Acknowledgement | Reply to the report email | within 3 days |
| Initial assessment | Severity / scope analysis | within 7 days |
| Fix development | Patch + tests | depends on severity |
| Public disclosure | CVE + release notes | after patch ships |

---

## dri's Security Model

### Compiler security

The dri compiler is expected to accept trusted source files.  
Bugs that let a hostile `.dri` file crash, hang, or pwn the compiler are treated
as security vulnerabilities.

Focus areas:

- **Parser robustness**: hostile input must not crash or loop forever.
- **Codegen integrity**: generated C++ must not introduce surprise behavior.
- **`use` path resolution**: module imports must not load files from disallowed
  paths.
- **`sys.exec` injection**: be careful when user input flows into `sys.exec()`.

### Security of generated code

dri-generated C++ guarantees:

- Memory safety via the `Own<T>` / `Ref<T>` / `Borrow<T>` ownership model.
- Leak prevention via `@region` arena allocation.
- Array bounds checks (enabled with `--debug` builds).

### Known limitations

- `sys.exec(cmd)` — passing user input directly is a shell-injection risk;
  always validate input first.
- `io.read_file(path)` — using unvalidated user input can lead to path
  traversal.
- `--release` builds disable bounds checks. When handling untrusted input,
  validate first with a `--debug` build.

---

## Security-related annotations

dri exposes annotations to make security and safety properties explicit:

```dri
@threadsafe
void safe_increment() {
    counter += 1;
};

@io
String read_config(String path) {
    return io.read_file(path);
};

@alloc
double[] create_buffer(int n) {
    return new double[n];
};
```

These effect annotations make it easier to spot risky operations during
code review.

---

## Acknowledgements

Thank you to everyone who responsibly discloses vulnerabilities. With your
permission, your name will be credited in the release notes.

---
