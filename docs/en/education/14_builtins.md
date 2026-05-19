# Built-in function reference
> The built-in functions provided by dri and the function list per namespace.

---

## Namespace rules

dri functions are divided into three tiers.

| Tier | Examples | Criterion |
|------|----------|----------|
| **Global** | `print`, `abs`, `max`, `sqrt`, `length` | Core operations that appear in almost every program |
| **Namespaced** | `math.pow`, `lst.sort`, `str.join`, `io.read_file` | Domain-specific extension functions |
| **tensor-only global** | `sum`, `dot`, `norm`, `mean` | Operate only on tensor types — the type system prevents conflicts |

> Criterion: **"Is it used in almost every dri file?"** → If YES, global; if NO, namespaced.

```dri
# Global — call directly with no namespace
abs(-5)         # 5
max(3, 7)       # 7
sqrt(16.0)      # 4.0
length("hello") # 5

# Namespaced — prefix required
math.pow(2.0, 10.0)     # 1024.0
lst.sort(fruits)         # sorted copy
str.join(words, ", ")    # string join
io.read_file("a.txt")    # file read
```

---

## Basic I/O

| Function | Signature | Description |
|----------|-----------|-------------|
| `print` | `print(...)` | Print values (space-separated, with newline) |
| `input` | `input(varName)` | Store standard input into a variable |

```dri
print("Hello,", "World!");   # Hello, World!
print(42, 3.14, true);

String s;
input(s);
print("Input:", s);
```

> See [[02_basics.md](02_basics.md)] for basic I/O syntax.

---

## Math functions (direct call)

| Function | Args | Description |
|----------|------|-------------|
| `abs(x)` | 1 | Absolute value |
| `max(a, b)` | 2 | Maximum |
| `min(a, b)` | 2 | Minimum |
| `sqrt(x)` | 1 | Square root |
| `floor(x)` | 1 | Floor |
| `ceil(x)` | 1 | Ceil |
| `round(x)` | 1 | Round |
| `sin(x)` | 1 | Sine (radians) |
| `cos(x)` | 1 | Cosine (radians) |
| `tan(x)` | 1 | Tangent (radians) |
| `fma(a, b, c)` | 3 | a\*b+c (Fused Multiply-Add) |
| `likely(x)` | 1 | Branch prediction hint: likely true |
| `unlikely(x)` | 1 | Branch prediction hint: likely false |

```dri
print(abs(-5));              # 5
print(max(3, 7));            # 7
print(min(3, 7));            # 3
print(sqrt(16.0));           # 4.0
print(floor(3.7));           # 3.0
print(ceil(3.2));            # 4.0
print(round(3.5));           # 4.0
print(sin(3.14159 / 2.0));   # ~1.0
print(fma(3.0, 4.0, 5.0));   # 17.0
```

---

## Collection commons

| Function | Args | Description |
|----------|------|-------------|
| `length(x)` | 1 | Array/string length |
| `split(s, delim)` | 2 | Split string → `list<String>` |

```dri
int[] arr = [1, 2, 3];
print(length(arr));         # 3
print(length("hello"));     # 5

list<String> parts = split("a,b,c", ",");
for (p of parts) { print(p); };
# a, b, c
```

---

## String conversion built-ins

> `toUpper`, `toLower`, `trim`, `replace` are **direct-call** functions, not `str.*`.

| Function | Args | Description |
|----------|------|-------------|
| `toUpper(s)` | 1 | Convert to uppercase |
| `toLower(s)` | 1 | Convert to lowercase |
| `trim(s)` | 1 | Trim surrounding whitespace |
| `replace(s, old, new)` | 3 | Replace substring |

```dri
print(toUpper("hello"));                      # HELLO
print(toLower("WORLD"));                      # world
print(trim("  hi  "));                        # "hi"
print(replace("foo bar", "bar", "baz"));      # foo baz
```

---

## str.* namespace

| Function | Args | Description |
|----------|------|-------------|
| `str.char_at(s, i)` | 2 | i-th character |
| `str.from_char(c)` | 1 | char → String |
| `str.from_int(n)` | 1 | int → String |
| `str.to_int(s)` | 1 | String → int |
| `str.substr(s, start, len)` | 3 | Substring |
| `str.starts_with(s, prefix)` | 2 | Prefix check |
| `str.ends_with(s, suffix)` | 2 | Suffix check |
| `str.contains(s, sub)` | 2 | Containment check |
| `str.repeat(s, n)` | 2 | Repeat n times |
| `str.simd_find(s, t)` | 2 | SIMD substring search (index, -1 if not found) |

```dri
print(str.char_at("hello", 1));               # 'e'
print(str.from_char('A'));                    # "A"
print(str.from_int(42));                      # "42"
print(str.to_int("99"));                      # 99
print(str.substr("hello world", 6, 5));       # "world"
print(str.starts_with("dri lang", "dri"));    # true
print(str.ends_with("file.dri", ".dri"));     # true
print(str.contains("hello world", "world"));  # true
print(str.repeat("ha", 3));                   # "hahaha"
print(str.simd_find("hello world", "world")); # 6
```

---

## lst.* namespace (list operations)

> It is **`lst.*`**, not `list.*`.

| Function | Args | Description |
|----------|------|-------------|
| `lst.push(l, val)` | 2 | Append to end |
| `lst.pop(l)` | 1 | Remove and return last element |
| `lst.size(l)` | 1 | Size (long) |
| `lst.get(l, i)` | 2 | i-th element |
| `lst.set(l, i, val)` | 3 | Modify i-th element |

```dri
list<int> nums = [];
lst.push(nums, 10);
lst.push(nums, 20);
lst.push(nums, 30);
print(lst.size(nums));       # 3
print(lst.get(nums, 1));     # 20
lst.set(nums, 1, 99);
int v = lst.pop(nums);       # 30
print(lst.size(nums));       # 2
```

---

## map.* namespace

| Function | Args | Description |
|----------|------|-------------|
| `map.set(m, key, val)` | 3 | Insert/update |
| `map.get(m, key)` | 2 | Look up value |
| `map.get_or(m, key, default)` | 3 | Look up with default |
| `map.has(m, key)` | 2 | Key existence |
| `map.del(m, key)` | 2 | Delete key |
| `map.keys(m)` | 1 | List of all keys |
| `map.size(m)` | 1 | Number of entries (long) |

```dri
Map<String, int> m = {};
map.set(m, "a", 1);
map.set(m, "b", 2);
print(map.get(m, "a"));          # 1
print(map.get_or(m, "c", 0));   # 0
print(map.has(m, "b"));          # true
map.del(m, "b");
print(map.size(m));              # 1
```

---

## tensor / statistics functions (direct call)

| Function | Args | Description |
|----------|------|-------------|
| `sum(t)` | 1 | Sum of elements (AVX auto-dispatch) |
| `dot(a, b)` | 2 | Dot product |
| `norm(t)` | 1 | Euclidean norm |
| `mean(t)` | 1 | Mean |
| `std(t)` | 1 | Standard deviation |
| `median(t)` | 1 | Median |
| `min_index(t)` | 1 | Index of minimum |
| `max_index(t)` | 1 | Index of maximum |
| `avx512_sum(t)` | 1 | Direct AVX-512 sum |
| `avx512_dot(a, b)` | 2 | Direct AVX-512 dot product |
| `fast_csv_read(path)` | 1 | Fast CSV parsing |

```dri
tensor<5, double> data = {3.0, 1.0, 4.0, 1.0, 5.0};
print(sum(data));          # 14.0
print(mean(data));         # 2.8
print(norm(data));         # ~5.83
print(min_index(data));    # 1
print(max_index(data));    # 4
```

---

## simd.* namespace

| Function | Args | Description |
|----------|------|-------------|
| `simd.fmadd(a, b, c)` | 3 | a\*b+c (FMA instruction) |

```dri
double r = simd.fmadd(3.0, 4.0, 5.0);   # 17.0
```

---

## math.* namespace

| Function | Args | Description |
|----------|------|-------------|
| `math.invsqrt(x)` | 1 | Fast 1/sqrt(x) |

```dri
print(math.invsqrt(4.0));   # 0.5
```

---

## bits.* namespace

| Function | Args | Description |
|----------|------|-------------|
| `bits.popcount(n)` | 1 | Number of 1 bits |

```dri
print(bits.popcount(0b10110101));   # 5
```

---

## io.* namespace

| Function | Args | Description |
|----------|------|-------------|
| `io.read_file(path)` | 1 | Read file → String |
| `io.write_file(path, content)` | 2 | Write file (overwrite) → boolean |
| `io.append_file(path, content)` | 2 | Append to file → boolean |
| `io.mmap_read(path)` | 1 | Memory-mapped file read (direct access, no copy) |

```dri
io.write_file("out.txt", "Hello, dri!");
String content = io.read_file("out.txt");
print(content);   # Hello, dri!
io.append_file("out.txt", "\nSecond line");
```

> **Caution when using `io.mmap_read`**: Letting the returned mapping reference escape outside the `@region` block  
> causes a **Use-After-Free** vulnerability. The compiler's escape analysis blocks this as a compile error.  
> See [[10_memory_model.md — @region escape analysis](10_memory_model.md)] for details.

---

## sys.* namespace

| Function | Args | Description |
|----------|------|-------------|
| `sys.exec(cmd)` | 1 | Execute system command → int (exit code) |
| `sys.affinity(core_id)` | 1 | Pin the thread to a specific CPU core |
| `sys.get_branch_trace()` | 0 | Return current thread's branch history → `list<String>` |
| `sys.clear_branch_trace()` | 0 | Clear current thread's branch history |

```dri
int code = sys.exec("echo hello");
print("Exit code:", code);
sys.affinity(0);

# Branch tracing (requires @trace annotation)
list<String> trace = sys.get_branch_trace();
for (entry of trace) {
    print(entry);
};
sys.clear_branch_trace();
```

---

## mem.* namespace

| Function | Args | Description |
|----------|------|-------------|
| `mem.prefetch(ptr)` | 1 | Cache prefetch |

```dri
mem.prefetch(large_array[i + 64]);
```

---

## perf.*

| Function | Args | Description |
|----------|------|-------------|
| `perf.now()` | 0 | Current time (milliseconds, double) |

```dri
double t0 = perf.now();
# code to measure
double t1 = perf.now();
print("Elapsed:", t1 - t0, "ms");
```

---

## reflect.* namespace

| Function | Args | Description |
|----------|------|-------------|
| `reflect.type_of(x)` | 1 | Type name → String |
| `reflect.fields(obj)` | 1 | List of field names |
| `reflect.has_field(obj, name)` | 2 | Whether a field exists |
| `reflect.methods(obj)` | 1 | List of method names |

```dri
int x = 42;
print(reflect.type_of(x));   # "int"
```

---

## compile_eval

| Function | Args | Description |
|----------|------|-------------|
| `compile_eval(expr)` | 1 | Evaluate expression at compile time |

```dri
int BUF = compile_eval(1024 * 1024);   # 1048576
double C = compile_eval(3.14159 / 4.0);
```

---

## wait.*

| Statement | Description |
|-----------|-------------|
| `wait.tick(n);` | Wait n ticks |
| `wait.second;` | Wait 1 second |
| `wait.minute;` | Wait 1 minute |
| `wait.hour;` | Wait 1 hour |

```dri
wait.tick(100);
wait.second;
```

> See [[15_extensions.md](15_extensions.md)] for extension functions (math, str, lst, map extensions).

---
