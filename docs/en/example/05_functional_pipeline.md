# Example 5: Functional Data Pipeline

> A declarative data transformation pipeline combining lambdas, pipe operators,
> HOF chains, and monadic patterns.

---

## Overview

- `|>` pipe operator for readable data-flow
- `Result<T>` for error propagation (monadic chaining)
- `filter/map/reduce` HOF chains
- Lambda captures + closure patterns
- `Option<T>` for null-safe handling

---

## Code

```dri
module data_pipeline;

# ── 1. Basic pipeline ──────────────────────────────────────────

list<int> numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

# traditional approach (harder to read)
list<int> evens = numbers.filter(|int x| -> x % 2 == 0);
list<int> squared = evens.map(|int x| -> x * x);
int total = squared.reduce(0, |int acc, int x| -> acc + x);

# pipe approach (declarative, readable)
int result = numbers
    |> numbers.filter(|int x| -> x % 2 == 0)
    |> numbers.map(|int x| -> x * x)
    |> numbers.reduce(0, |int acc, int x| -> acc + x);

print(`sum of squares of evens: ${result}`);  # 4+16+36+64+100 = 220

# ── 2. Composite lambda (closure) ─────────────────────────────

double make_multiplier(double factor) {
    # return a closure capturing factor
    return [copy factor] |double x| -> x * factor;
}

double double_it = make_multiplier(2.0);
double triple_it = make_multiplier(3.0);

double[] vals = [1.0, 2.0, 3.0, 4.0, 5.0];
double[] doubled = vals.map(double_it);
double[] tripled = vals.map(triple_it);

print("doubled:");
for (v of doubled) { print(v); }

# ── 3. Lambda composition ─────────────────────────────────────

# higher-order function to compose two functions
auto compose(auto f, auto g) {
    return [copy f, copy g] |auto x| -> f(g(x));
}

var add1  = |int x| -> x + 1;
var times2 = |int x| -> x * 2;
var add1_then_double = compose(times2, add1);   # (x+1)*2

list<int> seq = [1, 2, 3, 4, 5];
list<int> composed_result = seq.map(add1_then_double);
for (v of composed_result) { print(v); }   # 4,6,8,10,12

# ── 4. Result<T> monadic pipeline ────────────────────────────

# individual fallible transformation steps
Result<int> parse_int(String s) {
    if (str.is_empty(s)) {
        return Err("empty string");
    }
    # simple parsing simulation
    var is_valid = s.filter(|String c| -> c >= "0" and c <= "9");
    if (lst.length(is_valid) != str.length(s)) {
        return Err(`invalid number: ${s}`);
    }
    return Ok(str.to_int(s));
}

Result<int> validate_positive(int n) {
    if (n <= 0) { return Err(`must be positive: ${n}`); }
    return Ok(n);
}

Result<double> safe_sqrt(int n) {
    return Ok(math.sqrt(n));
}

# pipeline: parse → validate → compute
void process_input(String input) {
    Result<int> parsed = parse_int(input);
    match parsed {
        Ok(v) => {
            Result<int> validated = validate_positive(v);
            match validated {
                Ok(n) => {
                    Result<double> result = safe_sqrt(n);
                    match result {
                        Ok(r) => { print(`√${n} = ${r}`); };
                        Err(e) => { print(`sqrt error: ${e}`); };
                    };
                };
                Err(e) => { print(`validation error: ${e}`); };
            };
        };
        Err(e) => { print(`parse error: ${e}`); };
    };
}

process_input("16");     # √16 = 4.0
process_input("-5");     # validation error: must be positive: -5
process_input("abc");    # parse error

# ── 5. Option<T> safe chain ───────────────────────────────────

Map<String, String> config = {};
map.set(config, "host", "localhost");
map.set(config, "port", "8080");
# "timeout" is not set

String get_config(String key) {
    if (map.has(config, key)) {
        return map.get(config, key);
    }
    return "";
}

# handle missing keys with Option
Option<String> host = Some(get_config("host"));
Option<String> port = Some(get_config("port"));
Option<String> timeout = None;

match host {
    Some(h) => {
        match port {
            Some(p) => {
                String url = `http://${h}:${p}`;
                match timeout {
                    Some(t) => { print(`URL: ${url}, timeout: ${t}s`); };
                    None    => { print(`URL: ${url}, timeout: default`); };
                };
            };
            None => { print("no port"); };
        };
    };
    None => { print("no host"); };
};

# ── 6. Statistics pipeline ────────────────────────────────────

double[] data = [2.4, 5.1, 3.8, 9.2, 1.7, 6.3, 4.5, 8.1, 7.6, 3.2];

# remove outliers (IQR method)
double q1 = median(lst.take(lst.sort(data), lst.length(data) / 2));
double q3 = median(lst.slice(lst.sort(data), lst.length(data)/2, lst.length(data)));
double iqr = q3 - q1;
double lower = q1 - 1.5 * iqr;
double upper = q3 + 1.5 * iqr;

double[] clean = data.filter(|double x| -> x >= lower and x <= upper);

print(`original count: ${lst.length(data)}`);
print(`cleaned count: ${lst.length(clean)}`);
print(`mean: ${mean(clean)}`);
print(`std dev: ${std(clean)}`);
print(`median: ${median(clean)}`);

# Z-score normalisation
double mu  = mean(clean);
double sig = std(clean);
double[] normalized = clean.map([copy mu, copy sig] |double x| -> (x - mu) / sig);
print("normalized data:");
for (v of normalized) {
    print(`  ${v}`);
}
```

---

## Functional Pattern Summary

| Pattern | dri code | Description |
|---------|----------|-------------|
| Pipe | `x \|> f` | Same as `f(x)` |
| Map | `.map(f)` | Transform each element |
| Filter | `.filter(pred)` | Keep elements matching predicate |
| Reduce | `.reduce(init, f)` | Fold operation |
| Compose | `compose(f, g)` | `x → g(x) → f(g(x))` |
| Closure | `[copy x] \|y\| -> ...` | Capture outer variable |

---

## Key Takeaways

- `compose` function for lambda composition (`f ∘ g`)
- `Result<T>` chain for error propagation (short-circuit on failure)
- `[copy factor]` vs `[ref factor]` capture semantics
- Functional implementation of a statistics pipeline
