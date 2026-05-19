# Functional Programming
> Build data transformation pipelines with lambda expressions, higher-order functions (map/filter/reduce), and the pipe operator.

---

## Lambda Expressions

```
|parameter, ...| -> expression
|parameter, ...| { body }
```

```dri
|x| -> x * 2
|x, y| -> x + y
|x| -> x > 0
|| -> 42             # no parameters
```

Block body:

```dri
|x| {
    int doubled = x * 2;
    return doubled + 1;
}
```

### Capture Guide — `[copy]` / `[ref]`

Since dri transpiles to C++, you must specify **how to capture** when a lambda references outer variables.
If you do not specify, the compiler analyzes lifetimes and decides automatically, but capture guides are recommended for clear intent.

```
[copy variable, ...] |parameter| -> expression    # capture by value (copy)
[ref variable, ...]  |parameter| -> expression    # capture by reference (mind the lifetime)
```

```dri
int factor = 10;

# value copy — even if factor changes later, the lambda result is unaffected
var mul_copy = [copy factor] |int x| -> x * factor;

# reference capture — no copy overhead, but factor must outlive the lambda
var mul_ref = [ref factor] |int x| -> x * factor;

print(mul_copy(5));   # 50
print(mul_ref(5));    # 50

factor = 99;
print(mul_copy(5));   # 50  — unchanged because it is a copy
print(mul_ref(5));    # 495 — reflects the current factor because it is a reference
```

| Capture method | C++ translation | When to use |
|----------|----------|----------|
| `[copy x]` | `[x]` | When the lambda lives long, or the original change is irrelevant |
| `[ref x]` | `[&x]` | When performance matters and the lifetime is guaranteed |
| Unspecified | `[=]` or analysis result | Simple one-off lambdas |

---

## List Higher-Order Functions

### .map() — transform each element

```dri
list<int> nums = [];
lst.push(nums, 1);
lst.push(nums, 2);
lst.push(nums, 3);
lst.push(nums, 4);
lst.push(nums, 5);

var doubled = nums.map(|x| -> x * 2);
# [2, 4, 6, 8, 10]

var squared = nums.map(|x| -> x * x);
# [1, 4, 9, 16, 25]
```

### .filter() — filter by condition

```dri
var evens = nums.filter(|x| -> x % 2 == 0);
# [2, 4]

var big = nums.filter(|x| -> x > 3);
# [4, 5]
```

### .reduce() — accumulate and fold

```dri
int sum = nums.reduce(0, |acc, x| -> acc + x);
# 15

int product = nums.reduce(1, |acc, x| -> acc * x);
# 120

int max_val = nums.reduce(0, |acc, x| -> max(acc, x));
# 5
```

### .take() — first N elements

```dri
var first3 = nums.take(3);
# [1, 2, 3]
```

---

## Higher-Order Function Chaining

Chain higher-order functions to build a data pipeline.

```dri
list<int> nums = [];
lst.push(nums, 1);
lst.push(nums, 2);
lst.push(nums, 3);
lst.push(nums, 4);
lst.push(nums, 5);
lst.push(nums, 6);
lst.push(nums, 7);
lst.push(nums, 8);
lst.push(nums, 9);
lst.push(nums, 10);

# pick even numbers, square them, then sum
int result = nums
    .filter(|x| -> x % 2 == 0)
    .map(|x| -> x * x)
    .reduce(0, |acc, x| -> acc + x);
# 220 = 4+16+36+64+100
print(result);
```

---

## Pipe Operator — `|>`

```
expression |> function
```

`value |> f` is equivalent to `f(value)`.

```dri
int result = 5 |> |x| -> x * 2;    # 10
```

Chaining:

```dri
double r = 16.0
    |> |x| -> sqrt(x)          # 4.0
    |> |x| -> x * 3.14159;     # 12.566...
print(r);
```

String processing pipeline:

```dri
String processed = "  Hello, World!  "
    |> |s| -> trim(s)
    |> |s| -> toLower(s);
print(processed);   # "hello, world!"
```

### Type Inference Limits — Explicit Type Annotations

For single-parameter pipelines, the compiler infers types automatically.
**When generics or higher-order functions sit in the middle, inference may fail.** In that case, annotate the lambda parameter with a type.

```dri
# inferable: the preceding expression is int → x is determined to be int
int r = 5 |> |x| -> x * 2;

# not inferable: list<T>.map cannot fix T
# compile error — x's type is unknown
var result = some_list |> |x| -> x * 2;

# fix: annotate the lambda parameter
var result = some_list |> |int x| -> x * 2;
```

Explicit-type lambda syntax:

```
|type parameter| -> expression
|type parameter, type parameter| -> expression
```

```dri
# complex pipeline — explicit types
list<double> r = raw_data
    |> |list<int> l| -> l.filter(|int x| -> x > 0)
    |> |list<int> l| -> l.map(|int x| -> x as double * 1.5);
```

> If the compiler cannot infer types without annotations, it emits a **clear error message**.
> Simple transformation pipelines mostly work without annotations.

---

## lazy — Lazy Evaluation

Adding the `lazy` keyword defers evaluation until the value is actually used.

```dri
lazy list large_data = expensive_computation();
# expensive_computation() runs only when large_data is actually used
```

---

## Practical Examples

### Data Aggregation

```dri
list<double> temps = [];
lst.push(temps, 22.5);
lst.push(temps, 18.3);
lst.push(temps, 25.7);
lst.push(temps, 20.1);
lst.push(temps, 23.8);

double total = temps.reduce(0.0, |acc, x| -> acc + x);
double avg = total / lst.size(temps) as double;
print("Average:", avg);

double max_t = temps.reduce(temps[0], |acc, x| -> max(acc, x));
print("Max:", max_t);

var hot = temps.filter(|t| -> t > 23.0);
print("Days at or above 23 degrees:", lst.size(hot));
```

### Grade Processing

```dri
list<int> scores = [];
lst.push(scores, 85);
lst.push(scores, 92);
lst.push(scores, 78);
lst.push(scores, 95);
lst.push(scores, 60);

var passed = scores.filter(|s| -> s >= 70);
int pass_sum = passed.reduce(0, |acc, x| -> acc + x);
double pass_avg = pass_sum as double / lst.size(passed) as double;
print("Passed:", lst.size(passed));
print("Pass average:", pass_avg);
```

### String Processing

```dri
list<String> raw = [];
lst.push(raw, "  Alice  ");
lst.push(raw, "BOB");
lst.push(raw, "  charlie");

var clean = raw
    .map(|s| -> trim(s))
    .map(|s| -> toUpper(s))
    .filter(|s| -> length(s) > 3);

for (name of clean) {
    print(name);
};
# ALICE, CHARLIE
```

> For basic list collection usage, see [[05_collections.md](05_collections.md)].

---
