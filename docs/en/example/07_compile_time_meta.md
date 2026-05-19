# Example 7: Compile-Time Metaprogramming

> Combines `compile_eval`, `static_if`, and `dim` to complete complex
> calculations at compile time with zero runtime cost.

---

## Overview

- `compile_eval` : converts a function to `constexpr` → executed at compile time
- `static_if` : compile-time conditional branching (binary-distributable)
- `dim` + `compile_eval` : unit-conversion constants computed at compile time
- `-D` flags to select build profiles

---

## Code

```dri
module meta;

# ── 1. Compile-time math functions ───────────────────────────

compile_eval int fibonacci(int n) {
    if (n <= 1) { return n; }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

compile_eval int factorial(int n) {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}

compile_eval boolean is_prime(int n) {
    if (n < 2) { return false; }
    for (i in 2..n) {
        if (n % i == 0) { return false; }
    }
    return true;
}

# computed at compile time — used as constants at runtime
int fib20  = fibonacci(20);   # compile time: 6765
int fac12  = factorial(12);   # compile time: 479001600
boolean p7 = is_prime(7);     # compile time: true

print(`fib(20) = ${fib20}`);
print(`12! = ${fac12}`);
print(`is_prime(7) = ${p7}`);

# ── 2. Compile-time lookup table ─────────────────────────────

compile_eval int[] precompute_fibonacci_table(int n) {
    int[] table = lst.fill(n, 0);
    table[0] = 0;
    if (n > 1) { table[1] = 1; }
    for (i in 2..n) {
        table[i] = table[i-1] + table[i-2];
    }
    return table;
}

# Fibonacci table generated at compile time
int[] FIB_TABLE = precompute_fibonacci_table(30);
print(`fib(25) = ${FIB_TABLE[25]}`);  # O(1) lookup

# ── 3. Physical unit constants at compile time ────────────────

dim Meter   = "m";
dim Foot    = "ft";
dim Celsius = "°C";
dim Kelvin  = "K";

# unit conversion constants (computed at compile time)
compile_eval double feet_to_meters(double feet) {
    return feet * 0.3048;
}

compile_eval double celsius_to_kelvin(double c) {
    return c + 273.15;
}

compile_eval double meters_to_feet(double meters) {
    return meters / 0.3048;
}

double EMPIRE_STATE_METERS = feet_to_meters(1454.0);  # compile time
double BOILING_KELVIN      = celsius_to_kelvin(100.0); # compile time
double MARATHON_FEET       = meters_to_feet(42195.0);  # compile time

print(`Empire State Building: ${EMPIRE_STATE_METERS} m`);
print(`boiling point: ${BOILING_KELVIN} K`);
print(`marathon: ${MARATHON_FEET} ft`);

# ── 4. static_if — per-build-profile code ────────────────────

static_if (DEBUG) {
    # debug build: verbose logging, bounds checks
    print("[DEBUG] built in debug mode");

    void checked_get(int[] arr, int idx) {
        if (idx < 0 or idx >= lst.length(arr)) {
            throw `index out of range: ${idx} / ${lst.length(arr)}`;
        }
    }

} else {
    static_if (RELEASE) {
        # release build: optimised, logging stripped
        print("[RELEASE] release mode");

    } else {
        # default build
        print("[DEFAULT] default build");
    }
}

# platform branching
static_if (WINDOWS) {
    String PATH_SEP = "\\";
    String CONFIG_DIR = "%APPDATA%\\myapp";
} else {
    static_if (MACOS) {
        String PATH_SEP = "/";
        String CONFIG_DIR = "~/Library/Application Support/myapp";
    } else {
        String PATH_SEP = "/";
        String CONFIG_DIR = "~/.config/myapp";
    }
}

# ── 5. Compile-time type specialisation ──────────────────────

# choose different algorithm based on T
compile_eval boolean is_floating_point_type(String type_name) {
    return type_name == "double" or type_name == "float";
}

void compute<T>(T a, T b) {
    static_if (EXACT_ARITHMETIC) {
        # integer precise computation
        print(`integer result: ${a + b}`);
    } else {
        # floating-point computation
        print(`real result: ${a + b}`);
    }
}

# ── 6. Compile-time Sieve of Eratosthenes ────────────────────

compile_eval int[] sieve_of_eratosthenes(int limit) {
    boolean[] is_prime_arr = lst.fill(limit + 1, true);
    is_prime_arr[0] = false;
    if (limit > 0) { is_prime_arr[1] = false; }

    for (i in 2..limit) {
        if (is_prime_arr[i]) {
            int j = i * i;
            while (j <= limit) {
                is_prime_arr[j] = false;
                j += i;
            }
        }
    }

    int[] primes = [];
    for (i in 2..limit) {
        if (is_prime_arr[i]) {
            lst.push(primes, i);
        }
    }
    return primes;
}

# all primes up to 100 computed at compile time
int[] PRIMES_100 = sieve_of_eratosthenes(100);

print(`prime count up to 100: ${lst.length(PRIMES_100)}`);
print("first 10:");
for (p of lst.take(PRIMES_100, 10)) {
    print(p);
}

# ── 7. Compile-time matrix size optimisation ─────────────────

compile_eval int next_power_of_2(int n) {
    int p = 1;
    while (p < n) { p = p * 2; }
    return p;
}

int OPTIMAL_SIZE = next_power_of_2(500);   # compile time: 512
print(`optimal matrix size (500→): ${OPTIMAL_SIZE}`);
```

---

## Compile-Time vs Runtime Comparison

| Computation | Without | With `compile_eval` |
|-------------|---------|---------------------|
| `fibonacci(20)` | 6765 recursive calls at runtime | computed once at compile time → constant |
| `sieve(100)` | computed each run | compile-time → array constant |
| `static_if (DEBUG)` | runtime `if` | completely included/excluded at compile time |

---

## Build Commands

```bash
# debug build
dri main.dri --exe app -DDEBUG

# release build
dri main.dri --exe app --release -DRELEASE

# Windows build
dri main.dri --exe app -DWINDOWS

# multiple flags at once
dri main.dri --exe app --release -DRELEASE -DWINDOWS
```

---

## Key Takeaways

- `compile_eval` = automatic conversion to `constexpr` (C++)
- `static_if (FLAG)` = branch completely removed from the binary
- Lookup table pattern: O(n) preprocessing → O(1) runtime access
- `dim` + `compile_eval` = unit conversion errors blocked at compile time
