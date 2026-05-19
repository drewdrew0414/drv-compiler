# Basic Syntax — Comments, Variables, Types, I/O
> Every statement ends with a semicolon.

---

## Comments
```dri
# Use "#" for a comment.
!# For multi-line comments,
   start with "!#"
   and end with "##".
##
```

---

## Variable Declarations
```dri
int age = 25;
double pi = 3.14159;
String name = "Alice";
boolean flag = true;
char grade = 'A';
long population = 8000000000;
```

### Type Inference — ```var```
```dri
var x = 42;        # inferred as int
var y = 3.14;      # inferred as double
var msg = "hello"; # inferred as String
```

### Where `var` Is Allowed

| Location | `var` allowed? | Reason |
|------|----------------|------|
| Local variables | Yes | Type can be determined from the right-hand expression |
| for-loop variables | Yes | Type can be determined from the collection's element type |
| **Function return type** | No | The compiler cannot type-check the call site |
| **Function parameters** | No | Overload/generic resolution becomes impossible, and transpilation cost grows rapidly |
| **Class fields** | No | Struct layout cannot be determined at compile time |

```dri
# Allowed
var result = compute();           # local variable
for (var item of list) { ... };   # loop variable

# Compile error
var add(var a, var b) { ... };    # error: var not allowed for parameters/return type

class Foo {
    var value;                     # error: var not allowed in fields
};
```

> Because dri is an HPC language, function signatures **must always specify types explicitly**.
> This lets the C++ transpiler unambiguously determine ABI and overloads.

---

## Types
### Primitive Types
| Type | Size | Example |
|------|------------|------|
| `int` | 32-bit integer | `int n = 0;` |
| `long` | 64-bit integer | `long n = 0;` |
| `float` | 32-bit floating point | `float f = 0.0;` |
| `double` | 64-bit floating point | `double d = 0.0;` |
| `char` | 8-bit character | `char c = 'A';` |
| `String` | Mutable string | `String s = "";` |
| `boolean` | 8-bit true/false | `boolean b = true;` |
| `void` | — | Function return type only |

### Special Types
| Type | Storage | Example |
| --- |-----| --- |
| `arg` | Multiple Strings > list | `arg a = "h", "e", "l", "l", "o"` |

### Collection Types
| Type | C++ equivalent                    | Description | Example                              |
| --- |---------------------------| --- |---------------------------------|
| `array` / `int[]` | `std::vector<T>`          | Dynamic array | [See example](#array-/-int[]-사용-예시) |
| `list<T>` | `	std::vector<T>`         | Generic list | [See example](#list<T>-사용-예시)       |
| `Map<K, V>` | `std::unordered_map<K,V>` | Hash map | [See example](#Map<K,V>-사용-예시)       |
| `tensor` | `std::array / SIMD array`    | Numeric computation array | [See example](#tensor-사용-예시)       |
| `tensor<N,T>`	| `std::array<T,N>`           |	Statically sized tensor | [See example](#tensor<N,T>-사용-예시)       |

### Reference Types
### `Ref<T>` — Shared Ownership (`shared_ptr`)
```dri
Ref<String> s = new String();
Ref<MyClass> obj = new MyClass();
Ref<int[]> buf = new int[100];

# null is allowed
Ref<String> maybe = null;
if (maybe != null) {
    print(maybe);
};
```

### `Own<T>` — Unique Ownership (`unique_ptr`)

```dri
Own<int[]> buf = new int[1024];
Own<int[]> other = move buf;  # after the move, buf becomes unusable
```

### `Borrow<T>` — Non-owning Reference (`const T&`)

```dri
void read_only(Borrow<int[]> data) {
    print(data[0]);
};
```

## `Option<T>` — Present or Absent

```dri
Option<int> safe_div(int a, int b) {
    if (b == 0) {
        return None;
    };
    return Some(a / b);
};

match safe_div(10, 2) {
    Some(v) => { print("Result:", v); };
    None    => { print("Division failed"); };
};
```

## `Result<T>` — Success or Failure

```dri
Result<int> parse_num(String s) {
    if (length(s) == 0) {
        return Err("empty string");
    };
    return Ok(str.to_int(s));
};

match parse_num("42") {
    Ok(v)  => { print("Parsed successfully:", v); };
    Err(e) => { print("Error:", e); };
};
```

## `dim` — Physical Unit Type

```dri
dim Meter = "m";
dim Second = "s";

Meter dist = 5.0;
Second time = 2.0;
```

### Unit Arithmetic Rules

Arithmetic between units follows the rules below for its result type.

| Operation | Condition | Result Type |
|------|------|----------|
| `A + B`, `A - B` | Same unit | Same unit |
| `A + B`, `A - B` | Different units | **Compile error** |
| `A * B` | Any units | Composite unit (a new `dim` definition is required) |
| `A / B` | Any units | Composite unit (a new `dim` definition is required) or `double` |
| `A * scalar` | — | Same unit |
| `A / scalar` | — | Same unit |

```dri
dim Meter = "m";
dim Second = "s";
dim MeterPerSecond = "m/s";   # Pre-define the composite unit

Meter dist = 10.0;
Second time = 2.0;

# Same-unit addition — OK
Meter total = dist + Meter(3.0);   # 13.0 m

# Different-unit addition — compile error
# Meter wrong = dist + time;       # error: Meter + Second not allowed

# Division — receive as a composite unit, or as a double
MeterPerSecond speed = dist / time;   # 5.0 m/s (when the composite unit is defined)
double ratio = dist / time;           # 5.0 (unit dropped; allowed when received as double)

# Scalar multiplication/division
Meter half = dist * 0.5;    # 5.0 m
Meter scaled = dist / 2.0;  # 5.0 m
```

Undefined composite unit results:

```dri
# When a composite unit (dim) is not defined, the division result can only be received as double
dim Kg = "kg";
Meter dist2 = 3.0;
Kg mass = 5.0;

double ratio2 = dist2 / mass;   # OK: drops the unit and is received as double
# Meter kg_ratio = dist2 / mass;  # error: undefined composite unit
```

## `Union` Type

```dri
Union<int, String> val = 42;
Union<int, String> val2 = "hello";
```

## `ref` — Reference Variable (`T&`)

```dri
int x = 10;
ref int y = x;   # y is a reference to x
y = 20;          # x also becomes 20
print(x);        # 20
```

## Reflection

```dri
int x = 42;
String t = reflect.type_of(x);   # "int"
print(t);
```

---

## Modifiers
```dri
const int MAX = 100;      # constant
unsigned int mask = 0xFF; # unsigned integer
mut int counter = 0;      # explicit mutability (variables are mutable by default)
```

| Modifier | Description |
| ----- | --- |
| const | constant |
| unsigned | unsigned integer |
| mut | explicit mutability (variables are mutable by default) |

---

## Input / Output
### Output
```dri
print("Hello World!");          # >>> Hello World!
print(42);                      # >>> 42
print(true);                    # >>> true

int x = 10;                     # >>> defines x = 10
String name = "dri";            # >>> defines name = "dri"
print(name, " version: ", x);   # >>> dri version: 10
print(name + " version: " + x); # >>> dri version: 10
print(`{name} version: {x}`);   # >>> dri version: 10
print(name * 3);                # >>> dridridri
```

### Input
```dri
String usrInput; # As shown on the left, the default value is null.
input("Enter a value >>> ", usrInput);
usrInput = input("Enter a value >>> ");
```
> When using `<variable> = input(...);`,
> the form is `input(<String>, <args>);`.
