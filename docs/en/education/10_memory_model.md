# Memory Model
> dri provides an ownership and borrow model inspired by Rust. Use `Own`, `Ref`, `Borrow`, and `@region` to control memory.

---

## Ownership Type Summary

| Type | C++ counterpart | Ownership | Release | Concurrent access |
|------|----------|--------|------|----------|
| `Own<T>` | `unique_ptr<T>` | Exclusive (movable) | Automatic | Single thread |
| `Ref<T>` | `shared_ptr<T>` | Shared | Automatic when count hits 0 | Immutable sharing allowed |
| `Borrow<T>` | `const T&` | None | Not released | Concurrent reads allowed |
| `mut Borrow<T>` | `T&` | None | Not released | Exclusive write |
| `ref T` | `T&` | None | Not released | Alias |
| `@region` | arena | Whole region | Bulk-released at block end | Escape forbidden |

> For the basic declaration syntax of `Ref<T>`, `Own<T>`, and `Borrow<T>`, see [[02_basics.md](02_basics.md)].

---

## Ref\<T\> — Shared Ownership (shared_ptr)

Several variables share the same object. Allows null.

```dri
Ref<String> a = new String();
Ref<MyClass> obj = new MyClass();

# null allowed
Ref<String> maybe = null;
if (maybe != null) {
    print(maybe);
};
```

Linked list example:

```dri
class Node {
    public int val;
    public Ref<Node> next;
};

Ref<Node> head = new Node();
head.val = 1;
head.next = new Node();
head.next.val = 2;
head.next.next = null;

Ref<Node> cur = head;
while (cur != null) {
    print(cur.val);
    cur = cur.next;
};
```

---

## Own\<T\> — Single Ownership (unique_ptr)

Only one variable owns the value at a time. Ownership transfer must always be marked with the **`move` keyword**.
Assigning an `Own<T>` without `move` is a **compile error** — implicit transfer is not allowed.

```dri
Own<int[]> buf = new int[1024];

# explicit move — ownership transfer
Own<int[]> owner2 = move buf;
# after this, buf is invalidated — no further access

print(buf[0]);   # compile error: 'buf' used after move
```

### move Rules

| Situation | Behavior |
|------|------|
| `move` explicit | Ownership transferred, source invalidated |
| `Own<T>` assignment without `move` | **Compile error** |
| Access to invalidated variable | **Compile error** |
| `Ref<T>` / `Borrow<T>` assignment | `move` not required (shared/borrowed) |

```dri
# error examples
Own<int[]> a = new int[64];
Own<int[]> b = a;         # error: cannot assign Own<T> without the move keyword
Own<int[]> c = move a;    # OK
print(a[0]);              # error: 'a' used after move
```

Ownership transfer into a function:

```dri
void consume(Own<int[]> data) {
    data[0] = 42;
    print(data[0]);
};

Own<int[]> arr = new int[512];
consume(move arr);         # arr's ownership moves into consume
consume(move arr);         # error: 'arr' used after move
```

Returning ownership from a function (handing it back):

```dri
Own<int[]> process(Own<int[]> data) {
    data[0] = 99;
    return move data;   # return ownership
};

Own<int[]> buf = new int[128];
buf = process(move buf);   # passed in then handed back — buf valid again
print(buf[0]);             # 99
```

### Compiler Error Message Specification

| Situation | Error message |
|------|-----------|
| `Own<T>` assignment without `move` | `error: cannot copy 'Own<T>' — use 'move' to transfer ownership` |
| Access after invalidation | `error: 'X' used after move at line N` |
| Reuse of moved variable | `error: value moved into 'Y', 'X' is no longer valid` |

---

## Borrow\<T\> — Non-Owning Reference (const T&)

Access read-only without copying. Ownership is not transferred.

```dri
void print_buf(Borrow<int[]> buf) {
    print(buf[0], buf[1]);
};

Own<int[]> data = new int[4];
data[0] = 10;
data[1] = 20;

print_buf(data);    # 10 20
# data still owns its memory
```

---

## mut Borrow\<T\> — Mutable Exclusive Borrow (T&, exclusive)

Unlike read-only `Borrow<T>`, only **one mutable borrow** may exist at a time.
This rule **prevents data races at compile time** in parallel contexts such as `parallel for`.

### Borrow Rules (Read-Write Lock Philosophy)

| State | Allowed |
|------|------|
| Multiple `Borrow<T>` immutable borrows concurrently | OK |
| A single `mut Borrow<T>` mutable borrow | OK |
| Another immutable/mutable borrow while a mutable borrow is live | Compile error |

```dri
Own<int[]> data = new int[1024];

void read_only(Borrow<int[]> b) { print(b[0]); };
void write(mut Borrow<int[]> b) { b[0] = 99; };

read_only(data);    # OK
read_only(data);    # OK — multiple immutable borrows allowed

write(data);        # OK
write(data);        # OK — sequential execution, not simultaneous mutable borrows

# compile error example — cannot create an immutable borrow while a mutable borrow is live
mut Borrow<int[]> w = data;
Borrow<int[]> r = data;   # error: cannot create r while w is alive
```

### `parallel for` Safety

```dri
int[] arr = [1, 2, 3, 4, 5];

# safe: each thread reads an independent element immutably
parallel for (item of arr) {
    print(item * 2);
};

# compile error: attempting to mutate arr itself inside the loop causes a mutable-borrow conflict
parallel for (item of arr) {
    arr[0] = 0;   # error: arr is mutably borrowed in parallel
};
```

---

## ref — Reference Variable (T&)

```dri
int x = 10;
ref int y = x;   # y is an alias for x
y = 20;          # x becomes 20 as well
print(x);        # 20
```

---

## @region — Arena Allocation

```
@region name {
    body
}
```

All memory allocated inside the block is released in bulk when the block ends.

```dri
@region scratch {
    int[] temp = [0, 0, 0, 0, 0, 0, 0, 0];
    double[] workspace = [1.0, 2.0, 3.0, 4.0];

    for (i in 0..8) {
        temp[i] = i * 2;
    };

    int s = 0;
    for (v of temp) {
        s += v;
    };
    print("sum:", s);
    # block ends: temp, workspace released in bulk
}
```

### Nested regions

```dri
@region outer {
    int[] big_buf = new int[10000];

    @region inner {
        int[] small_tmp = new int[64];
        # perform computation
        # inner ends: small_tmp released immediately
    }
    # big_buf is still valid
    # outer ends: big_buf released
}
```

### HPC Pattern — Per-Iteration Temporary Allocation

```dri
for (iter in 0..1000) {
    @region frame {
        double[] grad = new double[1024];
        double[] update = new double[1024];

        simd for (i in 0..1024) {
            grad[i] = compute_grad(i, iter);
            update[i] = grad[i] * 0.01;
        };
        # frame ends: bulk release per iteration without heap fragmentation
    }
};
```

### @region Escape Analysis

If memory allocated inside an `@region` block, or a file mapping obtained from `io.mmap_read`,
**is assigned (escapes) to a variable outside the block, it is a compile error**.
Since the memory is released when the block ends, an outside reference would cause a use-after-free vulnerability.

```dri
# error example — letting an mmap result escape the region
Ref<var> escaped;

@region frame {
    var mapped = io.mmap_read("huge.bin");
    escaped = mapped;   # compile error: mapped escapes @region frame
}
# at this point frame ends → mapped is released
# escaped is a dangling pointer to released memory
print(escaped);   # use-after-free
```

```dri
# correct pattern — use only inside the block
@region frame {
    var mapped = io.mmap_read("huge.bin");
    process(mapped);          # referenced only inside the block
    double result = sum(mapped);
    print("energy:", result);
    # frame ends: mapped released safely
}
```

> Escape analysis is performed automatically by the compiler. When an escape is detected:
> `error: 'mapped' escapes @region 'frame' — use-after-free would occur`

---

## @stack — Force Stack Allocation

Places an array on the stack with no heap allocation overhead.

```dri
@stack
int[] local_buf = [0, 0, 0, 0, 0, 0, 0, 0];
# no heap allocation overhead
```

---

## @heap — Explicit Heap Allocation

```dri
@heap
int[] large = new int[10000000];
# explicitly documented as heap-allocated
```

---

## @align — SIMD/NUMA Memory Alignment

SIMD instructions (`simd for`, `simd.fmadd`) perform best when data is aligned on a specific byte boundary.
Misaligned access can cause performance degradation or crashes.

| SIMD instruction | Required alignment |
|------------|----------|
| SSE2 | 16 bytes |
| AVX2 | 32 bytes |
| AVX-512 | 64 bytes |

```dri
# force 64-byte alignment for AVX-512 optimization
@align(64)
double[] simd_buf = new double[1000000];

# 32-byte aligned struct for AVX2
@align(32)
class Vector3D {
    public double x;
    public double y;
    public double z;
};
```

> Translates to `alignas(N)` or `__attribute__((aligned(N)))` in the C++ output.

---

## compile_eval — Compile-Time Execution

Simple constant computation:

```dri
int BUF_SIZE = compile_eval(1024 * 1024);
double PI_4 = compile_eval(3.14159 / 4.0);
print(BUF_SIZE);   # 1048576
```

### compile_eval Function Block — Lookup Table Generation

When the `compile_eval` annotation is applied to a function, it becomes executable both at compile time and at runtime.
When assigned to a `const` variable, **it runs at compile time** and the result is embedded as a constant in the binary's data section.
This wraps C++17 `constexpr` perfectly.

```dri
compile_eval double[] generate_sin_table() {
    mut double[] table = new double[360];
    for (i in 0..360) {
        table[i] = math.sin(i as double * 0.017453);   # degrees → radians
    };
    return table;
};

# table generated at compile time → embedded as a constant in the binary (zero runtime overhead)
const double[] sin_table = generate_sin_table();

# instant runtime lookup
print(sin_table[90]);   # ~1.0
```

```dri
compile_eval int[] generate_primes(int limit) {
    mut int[] primes = [];
    for (n in 2..limit) {
        boolean is_prime = true;
        for (p of primes) {
            if (p * p > n) { break; };
            if (n % p == 0) { is_prime = false; break; };
        };
        if (is_prime) { lst.push(primes, n); };
    };
    return primes;
};

const int[] small_primes = generate_primes(1000);   # computed at compile time
```

---

## Effect Annotations

Declare what side effects a function has.

```dri
@alloc
int[] make_buf(int n) {
    return new int[n];
};

@io
void read_line() {
    String s;
    input(s);
};

@threadsafe
void increment() {
    counter += 1;
};
```

---

## atomic<T> — Atomic Variables

```dri
atomic<int>  counter = 0;      # → std::atomic<int32_t>
atomic<long> flag    = 0L;     # → std::atomic<int64_t>
```

### atomic<String> Notes

`atomic<String>` internally swaps **pointers** atomically. The string contents themselves are not updated atomically.

```dri
atomic<String> msg = "hello";

spawn {
    sys.sync.atomic_store(msg, "world");   # only the pointer is swapped atomically
};

String current = sys.sync.atomic_load(msg);
```

> **Compiler warning**: a warning is emitted automatically when `atomic<String>` is declared.
> If you need complete string atomicity, prefer `Ref<String>` + mutex.

---

## Incremental Build

Skips recompiling unchanged files in large projects.

```bash
dri main.dri --exe app --incremental          # stores hashes under .dri_cache/
dri main.dri --exe app --incremental --cache-dir /tmp/dri-cache
```

**How it works**:
1. Compare file modification time (mtime) → recompile immediately if clearly changed
2. mtime identical → compare CRC32 hashes → cache hit if contents match (skip recompile)
3. On success, record the hash in `.dri_cache/hashes.txt`

```
.dri_cache/
└── hashes.txt   # path + mtime + CRC32
```
