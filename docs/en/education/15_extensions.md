# Extension function reference
> In addition to the basic built-in functions, this chapter covers extension functions in the `math`, `str`, `lst`, `map`, `io`, and `sys` namespaces along with standalone utility functions.

---

## math.* — Extended math functions

| Function | Args | Returns | Description |
|----------|------|---------|-------------|
| `math.pow(x, n)` | 2 | double | x to the n-th power |
| `math.log(x)` | 1 | double | Natural log ln(x) |
| `math.log2(x)` | 1 | double | Log base 2 |
| `math.log10(x)` | 1 | double | Log base 10 |
| `math.exp(x)` | 1 | double | e^x |
| `math.asin(x)` | 1 | double | Inverse sine (radians) |
| `math.acos(x)` | 1 | double | Inverse cosine (radians) |
| `math.atan(x)` | 1 | double | Inverse tangent (radians) |
| `math.atan2(y, x)` | 2 | double | Inverse tangent (4-quadrant) |
| `math.hypot(x, y)` | 2 | double | sqrt(x²+y²) |
| `math.clamp(x, lo, hi)` | 3 | double | Clamp to lo ≤ x ≤ hi |
| `math.lerp(a, b, t)` | 3 | double | Linear interpolation: a+(b-a)\*t |
| `math.pi()` | 0 | double | π ≈ 3.14159... |
| `math.e()` | 0 | double | e ≈ 2.71828... |
| `math.sign(x)` | 1 | int | Sign: -1 / 0 / 1 |
| `math.degrees(x)` | 1 | double | radians → degrees |
| `math.radians(x)` | 1 | double | degrees → radians |
| `math.gcd(a, b)` | 2 | long | Greatest common divisor |
| `math.lcm(a, b)` | 2 | long | Least common multiple |
| `math.factorial(n)` | 1 | long | n! |
| `math.is_nan(x)` | 1 | boolean | Whether x is NaN |
| `math.is_inf(x)` | 1 | boolean | Whether x is infinity |
| `math.floor_div(a, b)` | 2 | long | Floor division floor(a/b) |
| `math.sinh(x)` | 1 | double | Hyperbolic sine |
| `math.cosh(x)` | 1 | double | Hyperbolic cosine |
| `math.tanh(x)` | 1 | double | Hyperbolic tangent |

```dri
double area = math.pi() * r * r;
double dist = math.hypot(3.0, 4.0);       # 5.0
double angle = math.atan2(1.0, 1.0);      # π/4
double clamped = math.clamp(x, 0.0, 1.0);
double lerped = math.lerp(0.0, 100.0, 0.5);  # 50.0
double powered = math.pow(2.0, 10.0);     # 1024.0
double logged = math.log(math.e());       # 1.0
int sign = math.sign(-5.0);              # -1

double deg = math.degrees(math.pi());     # 180.0
long g = math.gcd(12, 18);               # 6
long f = math.factorial(5);              # 120
long fd = math.floor_div(-7, 2);         # -4
double sh = math.sinh(1.0);              # ~1.175
```

---

## str.* — Extended string functions

| Function | Args | Returns | Description |
|----------|------|---------|-------------|
| `str.join(lst, sep)` | 2 | String | Join list of strings with sep |
| `str.index_of(s, sub)` | 2 | long | Position of substring (-1 if not found) |
| `str.pad_left(s, n)` | 2 | String | Pad left with spaces to length n |
| `str.pad_right(s, n)` | 2 | String | Pad right with spaces to length n |
| `str.is_empty(s)` | 1 | boolean | Whether the string is empty |
| `str.count(s, sub)` | 2 | long | Number of occurrences of sub |
| `str.reverse(s)` | 1 | String | Reverse string |
| `str.to_float(s)` | 1 | double | Parse string → double |
| `str.from_float(x)` | 1 | String | Convert double → string |
| `str.from_bool(b)` | 1 | String | `"true"` or `"false"` |
| `str.to_bool(s)` | 1 | boolean | `"true"`/`"1"` → true |
| `str.from_long(n)` | 1 | String | Convert long → string |
| `str.left(s, n)` | 2 | String | Leftmost n characters |
| `str.right(s, n)` | 2 | String | Rightmost n characters |
| `str.char_code(c)` | 1 | long | ASCII code |
| `str.from_char_code(n)` | 1 | String | ASCII code → character |
| `str.upper_first(s)` | 1 | String | Capitalize only the first character |
| `str.trim_left(s)` | 1 | String | Trim left whitespace only |
| `str.trim_right(s)` | 1 | String | Trim right whitespace only |
| `str.remove(s, sub)` | 2 | String | Remove all occurrences of sub |
| `str.truncate(s, n)` | 2 | String | Keep only the leading n characters |

```dri
list<String> words = [];
lst.push(words, "hello");
lst.push(words, "world");
lst.push(words, "dri");

String joined = str.join(words, ", ");   # "hello, world, dri"
print(joined);

long idx = str.index_of("hello world", "world");   # 6

print(str.pad_left("42", 6));     # "    42"
print(str.pad_right("hi", 5));    # "hi   "
print(str.is_empty(""));          # true
print(str.count("banana", "an")); # 2
print(str.reverse("hello"));      # "olleh"

double pi = str.to_float("3.14159");
print(str.from_float(pi));        # "3.14159"
print(str.from_bool(true));       # "true"
print(str.from_long(9999999999)); # "9999999999"

String s = "Hello, World!";
print(str.left(s, 5));            # "Hello"
print(str.right(s, 6));          # "World!"
print(str.upper_first("hello dri")); # "Hello dri"
print(str.trim_left("   hello"));    # "hello"
print(str.remove("banana", "an"));   # "ba"
print(str.truncate("Hello, World!", 5)); # "Hello"
```

---

## lst.* — Extended list functions

### Mutating functions (void)

| Function | Args | Description |
|----------|------|-------------|
| `lst.insert(l, i, val)` | 3 | Insert val at position i |
| `lst.remove(l, i)` | 2 | Remove element at index i |
| `lst.clear(l)` | 1 | Remove all elements |
| `lst.extend(l, other)` | 2 | Append other to the end of l |

```dri
list<String> fruits = [];
lst.push(fruits, "apple");
lst.push(fruits, "cherry");

lst.insert(fruits, 1, "banana");   # apple, banana, cherry
lst.remove(fruits, 0);             # banana, cherry

list<String> more = [];
lst.push(more, "date");
lst.push(more, "elderberry");
lst.extend(fruits, more);          # banana, cherry, date, elderberry

lst.clear(fruits);
print(lst.size(fruits));           # 0
```

### Query functions (return value)

| Function | Args | Returns | Description |
|----------|------|---------|-------------|
| `lst.contains(l, val)` | 2 | boolean | Whether val is contained |
| `lst.index_of(l, val)` | 2 | long | Index of val (-1 if not found) |
| `lst.first(l)` | 1 | value | First element |
| `lst.last(l)` | 1 | value | Last element |
| `lst.min(l)` | 1 | String | Minimum element (lexicographic order) |
| `lst.max(l)` | 1 | String | Maximum element (lexicographic order) |
| `lst.count(l, val)` | 2 | long | Number of occurrences of val |
| `lst.is_empty(l)` | 1 | boolean | Whether the list is empty |

```dri
list<String> nums = [];
lst.push(nums, "10");
lst.push(nums, "20");
lst.push(nums, "30");

print(lst.contains(nums, "20"));    # true
print(lst.index_of(nums, "20"));   # 1
print(lst.first(nums));            # "10"
print(lst.last(nums));             # "30"
print(lst.min(nums));              # "10"
print(lst.max(nums));              # "30"
print(lst.is_empty(nums));         # false
```

### Copy-returning functions

| Function | Args | Returns | Description |
|----------|------|---------|-------------|
| `lst.reverse(l)` | 1 | list | Reversed copy |
| `lst.sort(l)` | 1 | list | Sorted copy |
| `lst.join(l, sep)` | 2 | String | String joined with sep |
| `lst.slice(l, start, end)` | 3 | list | `[start..end)` slice |
| `lst.unique(l)` | 1 | list | Remove duplicates (preserving order) |
| `lst.fill(n, val)` | 2 | list | List filled with n copies of val |
| `lst.copy(l)` | 1 | list | Shallow copy |

```dri
list<String> data = [];
lst.push(data, "banana");
lst.push(data, "apple");
lst.push(data, "cherry");
lst.push(data, "apple");

var sorted = lst.sort(data);
print(lst.get(sorted, 0));        # "apple"

var rev = lst.reverse(data);
print(lst.get(rev, 0));           # "apple"  (first element of the reversed list)

String sentence = lst.join(data, " + ");
print(sentence);                   # "banana + apple + cherry + apple"

var part = lst.slice(data, 1, 3);
print(lst.size(part));            # 2

var unique_fruits = lst.unique(data);
print(lst.size(unique_fruits));   # 3

var zeros = lst.fill(5, "0");
print(lst.size(zeros));           # 5
```

---

## map.* — Extended map functions

| Function | Args | Returns | Description |
|----------|------|---------|-------------|
| `map.values(m)` | 1 | list | List of all values |
| `map.clear(m)` | 1 | void | Remove all entries |
| `map.contains_value(m, val)` | 2 | boolean | Whether the value is contained |
| `map.merge(m1, m2)` | 2 | Map | Copy of m1 overwritten by m2 |
| `map.copy(m)` | 1 | Map | Shallow copy |

```dri
Map<String, int> a = {};
map.set(a, "x", 1);
map.set(a, "y", 2);

Map<String, int> b = {};
map.set(b, "y", 99);
map.set(b, "z", 3);

print(map.contains_value(a, 1));    # true

var merged = map.merge(a, b);
# merged = {x:1, y:99, z:3}
print(map.get(merged, "y"));        # 99

var c = map.copy(a);
map.set(c, "w", 5);
print(map.has(a, "w"));             # false (original is unaffected)

map.clear(a);
print(map.size(a));                 # 0
```

---

## io.* — Filesystem extension functions

| Function | Args | Returns | Description |
|----------|------|---------|-------------|
| `io.exists(path)` | 1 | boolean | Whether the file/directory exists |
| `io.delete_file(path)` | 1 | boolean | Delete file |
| `io.file_size(path)` | 1 | long | File size in bytes (-1: error) |
| `io.list_dir(path)` | 1 | list\<String\> | Directory listing |
| `io.make_dir(path)` | 1 | boolean | Create directory (including intermediate paths) |

```dri
if (io.exists("data.txt")) {
    long size = io.file_size("data.txt");
    print("Size:", size, "bytes");
} else {
    print("File not found");
};

io.write_file("temp.txt", "temporary data");
io.delete_file("temp.txt");

io.make_dir("output/logs");

list<String> files = io.list_dir(".");
for (f of files) {
    print(f);
};
```

---

## sys.* — System extension functions

| Function | Args | Returns | Description |
|----------|------|---------|-------------|
| `sys.env(name)` | 1 | String | Environment variable value (empty string if missing) |
| `sys.exit(code)` | 1 | void | Terminate the program |
| `sys.sleep(ms)` | 1 | void | Wait ms milliseconds |
| `sys.time()` | 0 | long | Unix timestamp (seconds) |

```dri
String home = sys.env("HOME");
print("HOME:", home);

long t0 = sys.time();
# ... work ...
long t1 = sys.time();
print("Elapsed:", t1 - t0, "s");

sys.sleep(500);    # wait 0.5 seconds

int result = compute();
if (result < 0) {
    print("Error occurred");
    sys.exit(1);
};
sys.exit(0);
```

---

## Global utility functions

Functions that can be called directly without any namespace.  
For list manipulation such as `sort`, `reverse`, and `unique`, use the `lst.*` namespace.

| Function | Args | Returns | Description |
|----------|------|---------|-------------|
| `range(n)` | 1 | list | `["0","1",...,"n-1"]` list of strings |
| `range(start, end)` | 2 | list | `[start..end)` list of strings |
| `assert_that(cond, msg)` | 2 | void | Throw an exception when the condition fails |

```dri
# range — generate an integer-range list
var r = range(5);
for (s of r) {
    print(s);    # "0", "1", "2", "3", "4"
};

var r2 = range(3, 8);
for (s of r2) {
    print(s);    # "3", "4", "5", "6", "7"
};

# assert_that — precondition check
int divide(int a, int b) {
    assert_that(b != 0, "The denominator cannot be zero");
    return a / b;
};

try {
    int r = divide(10, 0);
} catch (err) {
    print("Exception:", err);   # Assertion failed: The denominator cannot be zero
};
```

> For list manipulation such as `sort`, `reverse`, `unique`, `join`, `min`, `max`, `index_of`, and `count`, use `lst.*`. See [[lst.* copy-returning functions](#lst-copy-returning-functions)].

> See [[14_builtins.md](14_builtins.md)] for basic built-in functions.

---
