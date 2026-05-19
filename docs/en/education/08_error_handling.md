# Error handling
> Three ways to handle errors: `try/catch` (exception-based), `Option<T>` (absent value), and `Result<T>` (success/failure with error information).

---

## try / catch

```
try {
    body
} catch (variable_name) {
    error handling
};
```

```dri
try {
    int result = 10 / 0;
    print(result);
} catch (err) {
    print("Error:", err);
};
```

---

## throw

```
throw expression;
```

`throw` throws a String message.

```dri
double divide(double a, double b) {
    if (b == 0.0) {
        throw "Cannot divide by zero";
    };
    return a / b;
};

try {
    double r = divide(10.0, 0.0);
    print(r);
} catch (err) {
    print("Exception:", err);
};
# Output: Exception: Cannot divide by zero
```

---

## Nested error handling

```dri
void process_file(String path) {
    if (length(path) == 0) {
        throw "Path is empty";
    };
    String content = io.read_file(path);
    if (length(content) == 0) {
        throw "File is empty";
    };
    print("Processing complete:", length(content), "bytes");
};

try {
    process_file("");
} catch (err) {
    print("Failed:", err);   # Failed: Path is empty
};

try {
    process_file("data.txt");
} catch (err) {
    print("Failed:", err);
};
```

---

## Option\<T\> — value present or absent

Used for lookup operations where a value may be missing. Returns `Some(value)` or `None`.

```dri
Option<int> safe_div(int a, int b) {
    if (b == 0) {
        return None;
    };
    return Some(a / b);
};

match safe_div(10, 2) {
    Some(v) => { print("Result:", v); };   # Result: 5
    None    => { print("Failed"); };
};

match safe_div(10, 0) {
    Some(v) => { print("Result:", v); };
    None    => { print("Failed"); };       # Failed
};
```

### Using Option for searches

```dri
Option<int> find_first_even(int[] arr) {
    for (n of arr) {
        if (n % 2 == 0) {
            return Some(n);
        };
    };
    return None;
};

int[] data = [1, 3, 5, 4, 7];
match find_first_even(data) {
    Some(v) => { print("First even:", v); };   # First even: 4
    None    => { print("No evens"); };
};
```

---

## Result\<T\> — success or failure (carries error information)

Used for conversion/validation operations that need to report a failure reason. Returns `Ok(value)` or `Err(message)`.

```dri
Result<int> parse_int(String s) {
    if (length(s) == 0) {
        return Err("Empty string");
    };
    return Ok(str.to_int(s));
};

match parse_int("42") {
    Ok(v)  => { print("Success:", v); };    # Success: 42
    Err(e) => { print("Error:", e); };
};

match parse_int("") {
    Ok(v)  => { print("Success:", v); };
    Err(e) => { print("Error:", e); };   # Error: Empty string
};
```

### Chained Result handling

```dri
Result<double> safe_sqrt(double x) {
    if (x < 0.0) {
        return Err("The square root of a negative number is not real");
    };
    return Ok(sqrt(x));
};

Result<double> compute(String input) {
    match parse_int(input) {
        Ok(n) => {
            match safe_sqrt(n as double) {
                Ok(v)  => { return Ok(v); };
                Err(e) => { return Err(e); };
            };
        };
        Err(e) => { return Err("Parse error: " + e); };
    };
};

match compute("25") {
    Ok(v)  => { print("Result:", v); };   # Result: 5.0
    Err(e) => { print("Error:", e); };
};

match compute("-4") {
    Ok(v)  => { print("Result:", v); };
    Err(e) => { print("Error:", e); };   # Error: The square root of a negative number is not real
};
```

---

## Mixing try with Option/Result

```dri
void run_job(String filename) {
    try {
        String content = io.read_file(filename);
        match parse_int(content) {
            Ok(n)  => { print("File value:", n * 2); };
            Err(e) => { throw "Parse failed: " + e; };
        };
    } catch (err) {
        print("Job failed:", err);
    };
};

run_job("number.txt");
run_job("missing.txt");
```

---

## Comparing error handling strategies

| Strategy | When to use |
|------|----------|
| `try/catch` | Accessing external resources (files, network) |
| `Option<T>` | Lookup operations where a value may be missing |
| `Result<T>` | Conversion/validation that needs to report a failure reason |

> For the `match` pattern matching syntax, see [[04_control_flow.md](04_control_flow.md)].

---
