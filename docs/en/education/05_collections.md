# Collections

---

## array

### Declaration and initialization

```dri
int[] nums = [1, 2, 3, 4, 5];
double[] prices = [1.99, 2.49, 3.99];
String[] names = ["Alice", "Bob", "Charlie"];
```

### Index access

```dri
int[] arr = [10, 20, 30, 40, 50];

print(arr[0]);    # 10
print(arr[4]);    # 50

arr[2] = 99;
print(arr[2]);    # 99
```

### Slicing — `arr[start..end]`

`start..end` covers values from start up to but **not including** end.

```dri
int[] arr = [10, 20, 30, 40, 50];

int[] a = arr[1..3];    # [20, 30]
int[] b = arr[2..4];    # [30, 40]
```

Using variables for slice bounds:

```dri
int s = 1;
int e = 4;
int[] slice = arr[s..e];   # [20, 30, 40]
```

### Length

```dri
int[] arr = [1, 2, 3, 4, 5];
print(length(arr));   # 5
```

### Iterating an array

```dri
int[] scores = [88, 92, 75, 100, 65];
int total = 0;

for (s of scores) {
    total += s;
};
double avg = total as double / length(scores) as double;
print("Average:", avg);
```

---

## list\<T\> (dynamic list)

> **Important**: list built-in functions use the `lst.` namespace. (Not `list.`.)

### Declaration

```dri
list<int> nums = [];
list<String> words = [];
list<double> values = [];
```

### lst.* operations

| Function | Description |
|------|------|
| `lst.push(l, val)` | Append to the end |
| `lst.pop(l)` | Remove and return the last element |
| `lst.size(l)` | Size (returns long) |
| `lst.get(l, i)` | The i-th element |
| `lst.set(l, i, val)` | Modify the i-th element |

```dri
list<String> lst = [];

lst.push(lst, "apple");
lst.push(lst, "banana");
lst.push(lst, "cherry");

print(lst.size(lst));          # 3
print(lst.get(lst, 0));        # apple
print(lst.get(lst, 1));        # banana

lst.set(lst, 1, "blueberry");
print(lst.get(lst, 1));        # blueberry

String removed = lst.pop(lst); # cherry
print(removed);
print(lst.size(lst));          # 2
```

### Iterating a list

```dri
list<int> nums = [];
lst.push(nums, 10);
lst.push(nums, 20);
lst.push(nums, 30);

for (n of nums) {
    print(n);
};
```

### Higher-order list functions

```dri
list<int> nums = [];
lst.push(nums, 1);
lst.push(nums, 2);
lst.push(nums, 3);
lst.push(nums, 4);
lst.push(nums, 5);

var doubled = nums.map(|x| -> x * 2);
var evens   = nums.filter(|x| -> x % 2 == 0);
int total   = nums.reduce(0, |acc, x| -> acc + x);
var first3  = nums.take(3);
```

---

## Map\<K, V\> (hash map)

### Declaration

```dri
Map<String, int> scores = {};
Map<String, String> config = {};
Map<int, boolean> flags = {};
```

### map.* operations

| Function | Description |
|------|------|
| `map.set(m, key, val)` | Insert/update |
| `map.get(m, key)` | Look up a value |
| `map.get_or(m, key, default)` | Look up with a default value |
| `map.has(m, key)` | Whether the key exists |
| `map.del(m, key)` | Delete a key |
| `map.keys(m)` | List of all keys (list\<K\>) |
| `map.size(m)` | Number of entries (long) |

```dri
Map<String, int> age = {};

map.set(age, "Alice", 30);
map.set(age, "Bob", 25);
map.set(age, "Charlie", 28);

int a = map.get(age, "Alice");            # 30
int x = map.get_or(age, "Dave", 0);      # 0 (default when missing)
boolean b = map.has(age, "Bob");          # true

map.del(age, "Charlie");
print(map.has(age, "Charlie"));           # false

print(map.size(age));                     # 2
```

### Map literals (initial values)

```dri
Map<String, int> m = {"a": 1, "b": 2, "c": 3};
```

### Iterating keys

```dri
Map<String, int> stock = {};
map.set(stock, "apple", 100);
map.set(stock, "banana", 50);
map.set(stock, "cherry", 200);

list<String> items = map.keys(stock);
for (item of items) {
    int count = map.get(stock, item);
    print(item, ":", count);
};
```

---

## tensor (numeric array)

### Statically-sized tensor

```dri
tensor<4, double> v = {1.0, 2.0, 3.0, 4.0};

print(sum(v));      # 10.0
print(norm(v));     # Euclidean norm
```

### Dynamic tensor (similar to a regular array)

```dri
tensor data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0];

print(sum(data));
print(mean(data));
print(std(data));
```

### tensor slicing view (Zero-Copy View)

When you slice part of a multidimensional tensor, the result is a view that points at the original memory **without allocating new memory**.
It transpiles to C++ `std::mdspan` or a strided pointer.

```
tensor[row range, col range]     # 2D slicing
tensor[start..end]               # 1D slicing (same as the existing array syntax)
```

```dri
# 100x100 matrix (10000 elements)
tensor<2, double> mat = new tensor<2, double>[100, 100];

# Create a view into rows 0..9, columns 0..9 without copying
var sub = mat[0..10, 0..10];   # No allocation, returns a view

print(mean(sub));   # Operate directly on the view

# The view shares memory with the original
sub[0, 0] = 99.0;
print(mat[0, 0]);   # 99.0 — reflected in the original
```

1D slicing:

```dri
tensor<8, double> v = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};

var first_half = v[0..4];   # View: 1.0 2.0 3.0 4.0
var second_half = v[4..8];  # View: 5.0 6.0 7.0 8.0

print(dot(first_half, second_half));   # Dot product
```

> Views follow `Borrow<tensor>` semantics — they cannot outlive the original.

### @layout_soa — Structure of Arrays layout

The default array layout is AoS (Array of Structures). Applying `@layout_soa` switches the
internal storage to SoA (Structure of Arrays) to maximize SIMD cache efficiency.

```dri
class Particle {
    public double x;
    public double y;
    public double z;
    public double mass;
};

# AoS: [x0,y0,z0,m0, x1,y1,z1,m1, ...]  — for intuitive access
Particle[] particles_aos = new Particle[1000];

# SoA: [x0,x1,...] [y0,y1,...] [z0,z1,...] [m0,m1,...]  — SIMD-optimized
@layout_soa
Particle[] particles_soa = new Particle[1000];

# Source code stays the same — only the compiler changes the internal layout
simd for (i in 0..1000) {
    particles_soa[i].x += particles_soa[i].mass * 0.01;
    # Internally: x[i] += mass[i] * 0.01  ← contiguous access, ideal for SIMD
};
```

| Layout | Memory | Use case |
|----------|--------|--------|
| AoS (default) | Each object's fields are contiguous | Frequent access to a single object |
| `@layout_soa` | Each field's array is contiguous | SIMD loops, GPU upload |

---

## `split` — string splitting

```dri
list<String> parts = split("a,b,c,d", ",");
for (p of parts) {
    print(p);
};
# a, b, c, d
```

---
