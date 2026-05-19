# Control Flow

> Every control structure uses a `{ }` brace block, and the statement as a whole ends with `;`.

---

## if / else if / else

```
if (condition) { body };
if (condition) { body } else { body };
if (condition) { body } else if (condition2) { body } else { body };
```

```dri
int score = 85;

if (score >= 90) {
    print("A");
} else if (score >= 80) {
    print("B");
} else if (score >= 70) {
    print("C");
} else {
    print("F");
};
# Output: B
```

```dri
int x = 15;
if (x > 10 and x < 20) {
    print("Between 10 and 20");
};

boolean logged_in = true;
boolean is_admin = false;
if (logged_in and not is_admin) {
    print("Regular user");
};
```

---

## for

```
for (variable in start..end) { body };
```

`start..end` is inclusive of `start` and **exclusive** of `end` (the endpoint is not included).

```dri
# 0, 1, 2, 3, 4
for (i in 0..5) {
    print(i);
};

# Sum
int sum = 0;
for (i in 1..101) {
    sum += i;
};
print("Sum of 1..100:", sum);   # 5050
```

### for — Iterating over a collection

```
for (variable of collection) { body };
```

```dri
int[] numbers = [10, 20, 30, 40, 50];
for (n of numbers) {
    print(n);
};
```

### for — Explicit C-style iteration

```
for (type variable = initial_value; condition; variable++) { body };
```

```dri
for (int i = 0; i < 10; i++) {
    print(i);
};

for (int i = 10; i > 0; i--) {
    print(i);
};

for (int i = 0; i < 100; i += 5) {
    print(i);
};
```

---

## while

```
while (condition) { body };
```

```dri
int count = 0;
while (count < 5) {
    print(count);
    count += 1;
};

# Infinite loop
int n = 1;
while (true) {
    if (n > 10) {
        break;
    };
    print(n);
    n += 1;
};
```

---

---

## break / continue / pass

```dri
# break — exits the loop
for (i in 0..10) {
    if (i == 5) {
        break;
    };
    print(i);
};
# Output: 0 1 2 3 4

# continue — skips the current iteration
for (i in 0..10) {
    if (i % 2 == 0) {
        continue;
    };
    print(i);
};
# Output: 1 3 5 7 9

# pass — does nothing (placeholder for an empty block)
for (i in 0..5) {
    pass;
};
```

---

## switch

```
switch (expression) {
    case (value) -> { body };
    case (value2) -> { body };
};
```

```dri
int day = 3;
switch (day) {
    case (1) -> { print("Monday"); };
    case (2) -> { print("Tuesday"); };
    case (3) -> { print("Wednesday"); };
    case (4) -> { print("Thursday"); };
    case (5) -> { print("Friday"); };
    case default -> { print("Holiday"); };
};
# Output: Wednesday
```

### String switch:

```dri
String cmd = "quit";
switch (cmd) {
    case ("start") -> { print("start"); };
    case ("stop")  -> { print("stop"); };
    case ("quit")  -> { print("quit"); };
};
```

---

## match — Pattern Matching

Used to destructure `Option<T>` and `Result<T>` values.

```
match expression {
    pattern => { body };
    pattern => { body };
};
```

Pattern kinds: `Some(variable)`, `None`, `Ok(variable)`, `Err(variable)`, `_` (wildcard), literals.

```dri
Option<int> maybe = Some(42);
match maybe {
    Some(v) => { print("Value:", v); };
    None    => { print("None"); };
};
# Output: Value: 42

Result<int> res = Ok(100);
match res {
    Ok(v)  => { print("Success:", v); };
    Err(e) => { print("Failure:", e); };
};
```

### Wildcard pattern:

```dri
match maybe {
    Some(v) => { print("Present:", v); };
    _       => { print("Absent or another pattern"); };
};
```

### Literal patterns:

```dri
int code = 404;
match code {
    200 => { print("OK"); };
    404 => { print("Not Found"); };
    500 => { print("Server Error"); };
    _   => { print("Other:", code); };
};
```

---

## parallel for — Parallel Iteration

Prepend a prefix to `for` to change the execution strategy.

```
parallel for (variable of collection) { body };
parallel for (variable in start..end) { body };
```

Each iteration runs in parallel on a work-stealing thread pool.

```dri
int[] data = [1, 2, 3, 4, 5, 6, 7, 8];

parallel for (item of data) {
    # Each iteration may run on a different thread
    print("Processing:", item);
};
```

When combined with `simd for`, you can apply parallelism and vectorization simultaneously:

```dri
parallel simd for (i in 0..N) {
    result[i] = a[i] * b[i] + c[i];
};
```

---

## simd for — SIMD Vectorized Iteration

```
simd for (variable in start..end) { body };
```

```dri
int N = 1024;
double[] a = new double[N];
double[] b = new double[N];
double[] c = new double[N];

simd for (i in 0..N) {
    c[i] = a[i] + b[i];
};
```

### where / otherwise — SIMD Mask Conditions

If you use a plain `if/else` inside a `simd for`, branch prediction breaks and SIMD vectorization is undone.
`where`/`otherwise` are converted into **AVX-512 mask registers** or hardware mask instructions such as `vblendv`.

```
simd for (variable in range) {
    where (mask_condition) {
        body_true
    } otherwise {
        body_false
    };
};
```

```dri
# No conditional branching — all SIMD lanes are processed with vector masks
simd for (i in 0..N) {
    where (x[i] > 0.0) {
        z[i] = sqrt(x[i]);
    } otherwise {
        z[i] = 0.0;
    };
};
# C++: converted into a form like z[i] = _mm512_mask_sqrt_pd(zero, mask, x[i])
```

Nesting multiple masks:

```dri
simd for (i in 0..N) {
    where (a[i] > 0.0 and b[i] < 1.0) {
        c[i] = a[i] * b[i];
    } otherwise {
        c[i] = 0.0;
    };
};
```

> `where`/`otherwise` are valid only inside a `simd for`.
> Inside a regular `for`, use a regular `if/else`.

---

## exists — Variable Existence Check

```
exists (variable_name) { body } else { body };
```

```dri
String data;
exists (data) {
    print("data exists:", data);
} else {
    print("data is missing");
};
```

---
