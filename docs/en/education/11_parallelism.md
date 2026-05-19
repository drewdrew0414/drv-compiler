# Parallelism and Asynchrony
> Process collections in parallel with `parallel for`, vectorize numeric loops with `simd for`, and handle asynchronous work with `async/await` and `spawn`.

---

## Loop Prefix System

dri declares execution behavior by prefixing `for`.

```
for (...)             { }   # ordinary sequential execution
parallel for (...)    { }   # work-stealing parallel execution
simd for (...)        { }   # CPU SIMD vectorization
parallel simd for (...) { } # parallel + vectorization combined
```

## Parallelism Selection Guide

| Situation | Method |
|------|------|
| Process collection elements independently | `parallel for (x of ...)` |
| Numeric-computation-heavy loop | `simd for (i in 0..N)` |
| Numeric computation + parallel together | `parallel simd for (i in 0..N)` |
| Background I/O | `async`/`await` |
| Background task that does not need a result | `spawn { }` |

---

## parallel for — Parallel Iteration

```
parallel for (variable of collection) { body };
parallel for (variable in start..end) { body };
```

Executes each iteration in parallel on a work-stealing thread pool.

```dri
int[] data = [1, 2, 3, 4, 5, 6, 7, 8];

parallel for (item of data) {
    print("processing:", item);
};
```

Large-scale parallel processing:

```dri
list<String> files = [];
lst.push(files, "a.txt");
lst.push(files, "b.txt");
lst.push(files, "c.txt");
lst.push(files, "d.txt");

parallel for (file of files) {
    String content = io.read_file(file);
    print("read:", file, "size:", length(content));
};
```

---

## simd for — SIMD Vectorization

```
simd for (variable in start..end) { body };
simd for (variable of collection) { body };
```

Automatically uses AVX2/AVX-512 (x86) or NEON (ARM) on the CPU.

```dri
int N = 1024;
double[] a = new double[N];
double[] b = new double[N];
double[] c = new double[N];

simd for (i in 0..N) {
    c[i] = a[i] + b[i];
};
```

Combined with `@noalias`, more aggressive vectorization is possible:

```dri
@noalias
void saxpy(double alpha, double[] x, double[] y, int n) {
    simd for (i in 0..n) {
        y[i] = alpha * x[i] + y[i];
    };
};
```

---

## parallel for reduction — Parallel Reduction

In a parallel loop, accumulating into a single variable from multiple threads causes a **data race (race condition)**.
The `reduction` keyword performs the operation on a per-thread local variable and **atomically merges** at loop end.

```
parallel for (variable of collection) reduction(operator:variable) { body };
```

| Operator | Initial value | Description |
|--------|--------|------|
| `+` | `0` | sum |
| `*` | `1` | product |
| `max` | smallest value | maximum |
| `min` | largest value | minimum |
| `and` | `true` | logical AND |
| `or` | `false` | logical OR |

```dri
int[] big_array = new int[1000000];
for (i in 0..1000000) { big_array[i] = i; };

mut int total = 0;
parallel for (item of big_array) reduction(+:total) {
    total += item;
};
print("sum:", total);

# maximum reduction
mut int max_val = 0;
parallel for (item of big_array) reduction(max:max_val) {
    if (item > max_val) { max_val = item; };
};
print("max:", max_val);
```

Multiple reductions at once:

```dri
mut double sum = 0.0;
mut double sum_sq = 0.0;

parallel for (x of data) reduction(+:sum) reduction(+:sum_sq) {
    sum += x;
    sum_sq += x * x;
};

double mean = sum / lst.size(data) as double;
double variance = sum_sq / lst.size(data) as double - mean * mean;
```

> Same semantics as C++ OpenMP's `reduction(+:sum)` pattern.

---

## parallel simd for — Parallel + Vectorization Combined

Applies parallel execution and SIMD vectorization at the same time. This is the core pattern for large-scale numeric computation.

```dri
int N = 1048576;   # 1M elements
double[] a = new double[N];
double[] b = new double[N];
double[] result = new double[N];

@noalias
parallel simd for (i in 0..N) {
    result[i] = a[i] * 2.0 + b[i];
};
```

The per-thread chunk size is automatically aligned to the SIMD width.

---

## async / await

Inside an `async` function, use `await` for non-blocking I/O.

```dri
async void fetch(String path) {
    String data = await io.read_file(path);
    print("data:", length(data), "bytes");
};

fetch("data.txt");
```

Sequentially executing multiple asynchronous tasks:

```dri
async void load_all() {
    String config = await io.read_file("config.json");
    String data   = await io.read_file("data.csv");
    print("config:", length(config), "bytes");
    print("data:", length(data), "bytes");
};

load_all();
```

async + try/catch:

```dri
async void safe_fetch(String path) {
    try {
        String content = await io.read_file(path);
        print("success:", length(content), "bytes");
    } catch (err) {
        print("read failure:", err);
    };
};

safe_fetch("existing.txt");
safe_fetch("missing.txt");
```

---

## spawn — Fire-and-Forget

```
spawn { body };
```

Runs in the background without waiting for the result.

```dri
spawn {
    print("background start");
    for (i in 0..1000000) {
        # heavy computation
    };
    print("background done");
};

print("main continues");
```

Starting several spawns concurrently:

```dri
spawn {
    String log = io.read_file("events.log");
    process_log(log);
};

spawn {
    String cfg = io.read_file("config.json");
    reload_config(cfg);
};

spawn {
    sys.exec("backup.sh");
};
```

---

## Performance Measurement

```dri
double t0 = perf.now();

parallel for (item of large_list) {
    heavy_work(item);
};

double t1 = perf.now();
print("elapsed:", t1 - t0, "ms");
```

---

## CPU Affinity

Pin threads to specific cores to improve NUMA/cache efficiency.

```dri
sys.affinity(0);    # pin to core 0

simd for (i in 0..1000000) {
    result[i] = compute(i);
};
```

---

## Full Example — Parallel Matrix Multiplication

```dri
int N = 256;
double[] A = new double[N * N];
double[] B = new double[N * N];
double[] C = new double[N * N];

# initialization
for (i in 0..N) {
    for (j in 0..N) {
        A[i * N + j] = i as double + j as double;
        B[i * N + j] = i as double - j as double;
    };
};

double t0 = perf.now();

# parallel for over rows
int[] rows = new int[N];
for (i in 0..N) {
    rows[i] = i;
};

parallel for (row of rows) {
    int i = row;
    for (j in 0..N) {
        double s = 0.0;
        simd for (k in 0..N) {
            s = simd.fmadd(A[i * N + k], B[k * N + j], s);
        };
        C[i * N + j] = s;
    };
};

double t1 = perf.now();
print("matrix multiplication done:", t1 - t0, "ms");
```

---

## Thread Safety Analyzer

The dri compiler runs a **static thread safety analysis** automatically at build time.

### Patterns It Detects

**1. Writes to shared variables without synchronization**

```dri
int total = 0;

parallel for (x of data) {
    total += x;   # warning: writing to total without atomic/reduction
};
```

Compiler warning:
```
warning: possible thread-unsafe write to 'total' inside parallel for
         — add reduction(+:total) or atomic<int>, or use sys.sync.fence_full()
```

**Fix 1: reduction**
```dri
int total = 0;
parallel for reduction(+:total) (x of data) {
    total += x;   # OK — translated to an OpenMP reduction
};
```

**Fix 2: atomic**
```dri
atomic<int> total = 0;
parallel for (x of data) {
    sys.sync.atomic_store(total, sys.sync.atomic_load(total) + x);
};
```

### Disabling the Analyzer

```bash
dri main.dri --exe app --no-analyze   # skip static analysis (faster release builds)
```

---

## Hardware-Topology-Aware Scheduler (sys.topology)

Modern CPUs group cores that share the same L3 cache into a die or CCD (Core Complex Die). Using `sys.topology.*`, you can recognize this structure and place threads at optimal positions.

### Intel P-core / E-core Recognition

```dri
use sys;

list<int> fast_cores = sys.topology.p_cores();  # list of performance core IDs
list<int> eff_cores  = sys.topology.e_cores();  # list of efficiency core IDs
int total = sys.topology.total();
String vendor = sys.topology.vendor();  # "Intel" / "AMD" / "other"

print("total logical cores:", total);
print("P-cores:", fast_cores);
print("E-cores:", eff_cores);
```

### AMD CCD (L3 Cache Sharing Group)

```dri
use sys;

# list of core groups sharing the L3 cache (AMD CCD, Intel Ring, etc.)
list<list<int>> groups = sys.topology.cache_groups();

for (i in 0..length(groups)) {
    print("cache group", i, ":", groups[i]);
};
```

### Parallel Processing Optimization Pattern

```dri
use sys;

# concentrate threads on L3-sharing cores before running parallel for
sys.topology.pin_threads();   # automatically pins to the optimal cache group

int[] data = range(1000000);
int total = 0;

parallel for reduction(+:total) (x of data) {
    total += x * x;
};

print("result:", total);
```

### sys.topology API

| Function | Return type | Description |
|------|--------|------|
| `sys.topology.p_cores()` | `list<int>` | list of performance (P) core IDs |
| `sys.topology.e_cores()` | `list<int>` | list of efficiency (E) core IDs |
| `sys.topology.cache_groups()` | `list<list<int>>` | L3 cache sharing groups |
| `sys.topology.total()` | `int` | total number of logical cores |
| `sys.topology.vendor()` | `String` | CPU vendor string |
| `sys.topology.pin_threads()` | `void` | pin the current thread to the optimal group |
| `sys.topology.set_affinity(n)` | `void` | pin the current thread to core n |

### Effects

- **Intel 12th gen and later**: only P-cores assigned to parallel loops → avoid the slower E-core frequency
- **AMD Ryzen (multi-CCD)**: concentrate threads on the same CCD → eliminate cross-CCD latency (~80 ns)
- **General**: place threads only within L3-sharing groups → cache misses plummet
