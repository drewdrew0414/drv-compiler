# Annotations
> Annotations use the `@` prefix to give the compiler hints about optimization, layout, and effects.

---

## Full list

| Annotation | Applies to | Description |
|------------|-----------|-------------|
| `@inline` | Function | Force inlining |
| `@pure` | Function | Pure function (no side effects) |
| `@fastcall` | Function | Fast calling convention |
| `@abstract` | Method | Force child class to implement |
| `@stack` | Variable | Force stack allocation |
| `@heap` | Variable | Explicit heap allocation |
| `@local` | Variable | Force stack/arena |
| `@align(N)` | Variable / class | Force N-byte alignment (SIMD/NUMA optimization) |
| `@packed` | Class | Tight layout with no padding (inheritance not allowed) |
| `@layout_soa` | Array variable | Force Structure of Arrays layout |
| `@noalias` | Function | No pointer aliasing (`restrict`) |
| `@io` | Function | Declare I/O effect |
| `@alloc` | Function | Declare memory allocation effect |
| `@threadsafe` | Function | Declare thread-safe |
| `@bench` | Function | Automatically measure execution time |
| `@specialize` | Function | Per-type SIMD specialization |
| `@trace` | Function | Auto-inject branch tracing instrumentation |
| `@warrant` | Function / rule | Declare Warrant relation in Toulmin argumentation |
| `@rebuttal` | Function / rule | Declare Rebuttal relation in Toulmin argumentation |
| `@defeats` | Function / rule | Declare Defeat relation in Toulmin argumentation |

---

## Function optimization

### @inline — Force inlining

Eliminates function call overhead.

```dri
@inline
int square(int x) {
    return x * x;
};

@inline
double clamp(double val, double lo, double hi) {
    if (val < lo) { return lo; };
    if (val > hi) { return hi; };
    return val;
};
```

### @pure — Pure function

Has no side effects and always returns the same output for the same input. The compiler may cache the result or eliminate the call.

```dri
@pure
double normalize(double x, double min, double max) {
    return (x - min) / (max - min);
};

@pure
int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    };
    return a;
};
```

### @fastcall — Fast calling convention

Apply to hot-path functions.

```dri
@fastcall
void process(int x, int y, int z) {
    print(x + y + z);
};

@fastcall
double lerp(double a, double b, double t) {
    return a + (b - a) * t;
};
```

---

## Memory layout

### @stack — Force stack allocation

```dri
void compute() {
    @stack
    double[] local_buf = [0.0, 0.0, 0.0, 0.0];

    simd for (i in 0..4) {
        local_buf[i] = i as double * 2.0;
    };
};
```

### @heap — Explicit heap allocation

```dri
@heap
double[] large = new double[10000000];
```

### @packed — Struct without padding

Removes padding between fields to save memory.

```dri
@packed
class Pixel {
    public char r;
    public char g;
    public char b;
    public char a;    # exactly 4 bytes
};

@packed
class Vertex {
    public float x;
    public float y;
    public float z;   # exactly 12 bytes
};
```

> **Constraint**: `@packed` is only allowed on **leaf data classes** with no inheritance.  
> Applying it to a class that uses `extends`, or inheriting from a `@packed` class, is a **compile error**.  
> Reason: the vtable pointer inserted by inheritance has its alignment broken by `@packed`,  
> causing misaligned access (performance loss or crash).

### @align(N) — SIMD/NUMA memory alignment

SIMD vector instructions reach peak performance when data is aligned to specific byte boundaries.

```dri
# AVX-512 optimization (64-byte alignment)
@align(64)
double[] simd_buf = new double[1000000];

# AVX2 optimization (32-byte alignment)
@align(32)
class Vector3D {
    public double x;
    public double y;
    public double z;
};
```

C++ translation: `alignas(N)` or `__attribute__((aligned(N)))`

| Alignment | Target SIMD |
|-----------|------------|
| `@align(16)` | SSE2 |
| `@align(32)` | AVX2 |
| `@align(64)` | AVX-512 |

### @layout_soa — Structure of Arrays layout

Stores arrays in SoA layout, ideal for SIMD loops or GPU upload.  
You write the source code as AoS, but the compiler transforms only the internal layout.

```dri
class Particle {
    public double x;
    public double y;
    public double z;
};

# Internally: split into three arrays double[] x, double[] y, double[] z
@layout_soa
Particle[] pts = new Particle[1000];

# Source code stays the same; internally SIMD-optimized as x[i] += ...
simd for (i in 0..1000) {
    pts[i].x += pts[i].mass * 0.01;
};
```

### @local — Force stack/arena

```dri
@local
int[] buf = new int[64];
```

---

## Vectorization

### @noalias — No pointer aliasing

Declares that different pointers do not point to the same memory. Combined with `simd for`, it enables more aggressive vectorization.

```dri
@noalias
void vec_add(double[] a, double[] b, double[] c, int n) {
    simd for (i in 0..n) {
        c[i] = a[i] + b[i];
    };
};

@noalias
void scale(double[] v, double factor, int n) {
    simd for (i in 0..n) {
        v[i] = v[i] * factor;
    };
};
```

---

## Effect system

Declares which side effects a function has.

### @io — Performs I/O

```dri
@io
String read_config(String path) {
    return io.read_file(path);
};

@io
void log(String msg) {
    io.append_file("app.log", msg + "\n");
    print(msg);
};
```

### @alloc — Memory allocation

```dri
@alloc
double[] create_buf(int n) {
    return new double[n];
};

@alloc
Map<String, int> build_index(list<String> keys) {
    Map<String, int> idx = {};
    for (i in 0..lst.size(keys)) {
        map.set(idx, lst.get(keys, i), i);
    };
    return idx;
};
```

### @threadsafe — Thread-safe

```dri
int counter = 0;

@threadsafe
void safe_increment() {
    counter += 1;
};

@threadsafe
int safe_get() {
    return counter;
};
```

---

## Abstract methods

### @abstract — Force implementation

Marks a method inside an abstract class that child classes must implement.

```dri
abstract class Codec {
    @abstract String encode(String input);
    @abstract String decode(String encoded);

    void verify(String original) {
        String enc = this.encode(original);
        String dec = this.decode(enc);
        if (dec == original) {
            print("Verification passed");
        } else {
            print("Verification failed");
        };
    };
};

class Base64Codec extends Codec {
    override String encode(String input) {
        return input;
    };

    override String decode(String encoded) {
        return encoded;
    };
};
```

---

## Combining annotations

Multiple annotations can be used together.

```dri
@inline
@pure
@fastcall
double fast_lerp(double a, double b, double t) {
    return a + (b - a) * t;
};

@noalias
@io
void stream_write(double[] data, int n, String path) {
    for (i in 0..n) {
        io.append_file(path, str.from_int(data[i] as int) + "\n");
    };
};

@alloc
@pure
int[] make_range(int n) {
    int[] arr = new int[n];
    for (i in 0..n) {
        arr[i] = i;
    };
    return arr;
};
```

> See [[06_functions_and_classes.md](06_functions_and_classes.md)] for examples of function declarations with annotations.

---

## Branch tracing — `@trace`

**Automatically injects instrumentation code** into every `if/else` and `switch/match` entry point inside the function.  
Upon entering each branch, it records `(function name, condition string, line number)` in a thread-local map.  
Use `sys.get_branch_trace()` to query the current thread's branch history.

```dri
@trace
Result<double> safe_divide(double a, double b) {
    if (b == 0.0) {
        return Err("Denominator is 0");
    };
    return Ok(a / b);
};

safe_divide(10.0, 0.0);

# Query branch history
list<String> trace = sys.get_branch_trace();
for (entry of trace) {
    print(entry);
};
# Output: "safe_divide | b == 0.0 → true | line 3"
```

In the C++ translation, code like `__dri_trace_push(__func__, "b == 0.0", __LINE__)` is injected before each branch.  
The `dri-toulmin` library uses this history to build argument-evidence trees.

---

## Toulmin argumentation annotations — `@warrant`, `@rebuttal`, `@defeats`

Integrating with the `dri-toulmin` library, these embed an **argumentation relation graph** between functions and rules as binary metadata.  
The runtime rule engine reads this metadata via reflection to compute confidence scores.

> Reference: [park-jun-woo/toulmin](https://github.com/park-jun-woo/toulmin)

| Annotation | Toulmin element | Meaning |
|------------|----------------|---------|
| `@warrant` | Warrant | "If this function is true, it supports the claim" |
| `@rebuttal` | Rebuttal | "If this function is true, it raises an exception to the claim" |
| `@defeats` | Defeat | "If this function is true, it neutralizes another rule" |

```dri
# Base claim (Claim)
boolean is_safe_speed(double speed_kmh) {
    return speed_kmh <= 100.0;
};

# Warrant: supports the claim that speeding is dangerous
@warrant("is_safe_speed")
boolean no_speeding(double speed_kmh) {
    return speed_kmh < 120.0;
};

# Rebuttal: exception on highways
@rebuttal("is_safe_speed")
boolean highway_exception(String road_type) {
    return road_type == "highway";
};

# Defeats: emergency vehicles neutralize the no_speeding rule
@defeats("no_speeding")
boolean emergency_vehicle(boolean is_emergency) {
    return is_emergency;
};
```

The compiler parses these annotations and embeds the following metadata in the binary:

```json
{
  "rules": [
    { "name": "no_speeding",       "type": "warrant",  "target": "is_safe_speed" },
    { "name": "highway_exception", "type": "rebuttal", "target": "is_safe_speed" },
    { "name": "emergency_vehicle", "type": "defeats",  "target": "no_speeding" }
  ]
}
```

> See [[17_external_libraries.md](17_external_libraries.md)] for `dri-toulmin` library integration and the confidence-score algorithm.

---

## Scope Annotations

Some annotations apply not only to a single function or variable but to an **entire block**. The compiler injects the corresponding pragma on block entry and automatically reverts it on block exit.

| Annotation | C++ translation | Description |
|------------|----------------|-------------|
| `@fast_math` | `#pragma GCC optimize("fast-math")` | Allow floating-point associativity, maximum vectorization |
| `@strict_math` | `#pragma STDC FENV_ACCESS ON` | Strict IEEE 754 compliance, exception detection |
| `@noalias` | `__restrict__` on parameters | Hints no aliasing on all pointer/reference parameters inside the function |

```dri
@fast_math
double[] dot_product_batch(double[] a, double[] b) {
    # fast-math optimization is enabled inside this function
    double[] result = [];
    for (i in 0..length(a)) {
        lst.push(result, a[i] * b[i]);
    };
    return result;
};
# #pragma GCC reset_options is auto-injected at function exit
```

```dri
@noalias
void matrix_multiply(double[] dst, double[] a, double[] b, int n) {
    # dst, a, b are declared non-overlapping → improved compiler vectorization
    parallel simd for (i in 0..n) {
        dst[i] = a[i] * b[i];
    };
};
```

> **Static analyzer**: After `@noalias` is declared, passing the same variable to two parameters triggers a compile error.

```dri
double[] v = [1.0, 2.0, 3.0];
matrix_multiply(v, v, v, 3);  # Error: @noalias violation — v is passed to dst, a, and b
```

---

## sys.sync — Memory barriers

```dri
use sys;

int shared_counter = 0;

spawn {
    sys.sync.atomic_store(shared_counter, 42);
    sys.sync.fence_release();   # → std::atomic_thread_fence(memory_order_release)
};

sys.sync.fence_acquire();       # → std::atomic_thread_fence(memory_order_acquire)
int val = sys.sync.atomic_load(shared_counter);
```

| Function | C++ translation | Description |
|----------|----------------|-------------|
| `sys.sync.fence_full()` | `atomic_thread_fence(seq_cst)` | Full sequential-consistency barrier |
| `sys.sync.fence_acquire()` | `atomic_thread_fence(acquire)` | Read barrier |
| `sys.sync.fence_release()` | `atomic_thread_fence(release)` | Write barrier |
| `sys.sync.atomic_load(x)` | `x.load(acquire)` | Atomic read |
| `sys.sync.atomic_store(x, v)` | `x.store(v, release)` | Atomic write |
| `sys.sync.atomic_cas(x, e, n)` | `x.compare_exchange_strong(e, n)` | CAS operation |
