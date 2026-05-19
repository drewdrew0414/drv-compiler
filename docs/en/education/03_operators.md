# Operators

---

## Arithmetic Operators

```dri
int a = 10;
int b = 3;

print(a + b);    # 13
print(a - b);    # 7
print(a * b);    # 30
print(a / b);    # 3.33333...
print(a // b);   # 3 (integer division)
print(a % b);    # 1  (remainder)
print(a ** b);   # 1000 (exponentiation)

double x = 10.0;
double y = 3.0;
print(x / y);
```
> When assigning the result of `//` to a variable, the variable must be of float type (to avoid a type error).

---

## Compound Assignment Operators

```dri
int n = 10;
n += 5;    # n = 15
n -= 3;    # n = 12
n *= 2;    # n = 24
n /= 4;    # n = 6
n %= 4;    # n = 2
```
> `++` and `--` are also available.

---

## Comparison Operators

```dri
int a = 5;
int b = 10;

print(a == b);   # false
print(a != b);   # true
print(a < b);    # true
print(a > b);    # false
print(a <= b);   # true
print(a >= b);   # false
```

---

## Logical Operators

`and`, `or`, and `not` are the canonical keywords. `&&`, `||`, and `!` are backward-compatible aliases, so they should be avoided in new code.

```dri
boolean t = true;
boolean f = false;

print(t and f);    # false
print(t or f);     # true
print(not t);      # false

int x = 15;
print(x > 10 and x < 20);   # true
print(x < 5 or x > 10);     # true
```

> `&&`, `||`, and `!` are recognized by the parser, but they are **deprecated** aliases.
> The keyword form was adopted as the canonical syntax to visually distinguish them from the bitwise operators `&` and `|`.

---

## Bitwise Operators

```dri
int a = 0b1100;   # 12
int b = 0b1010;   # 10

print(a & b);    # 0b1000 = 8  (AND)
print(a | b);    # 0b1110 = 14 (OR)
print(a ^ b);    # 0b0110 = 6  (XOR)
print(~a);       # bitwise inversion (NOT)
print(a << 2);   # 0b110000 = 48 (left shift)
print(a >> 1);   # 0b0110 = 6   (right shift)
```

| Operator | Name | Description |
|--------|------|------|
| `&` | AND | 1 only if both bits are 1 |
| `\|` | OR | 1 if either bit is 1 |
| `^` | XOR | 1 if the two bits differ |
| `~` | NOT | Bitwise inversion |
| `<<` | Left shift | Equivalent to multiplying by 2^n |
| `>>` | Right shift | Equivalent to dividing by 2^n |

---

## `in` Operator (Membership Test)

```dri
int[] arr = [1, 2, 3, 4, 5];
print(3 in arr);    # true
print(9 in arr);    # false
```

---

## `is` Operator

```dri
int x = 5;
print(x is 5);      # true (same as ==)
print(x is not 3);  # true (same as !=)
```

---

## Type Casting — `as`

```dri
int n = 42;
double d = n as double;     # 42.0
int back = d as int;        # 42 (fractional part dropped)

long big = 9999999999;
int small = big as int;     # beware of overflow
```

---

## Literal Forms

```dri
# Integers
int dec = 255;
int hex = 0xFF;       # Hexadecimal (255)
int bin = 0b11001100; # Binary

# Floating point
double pi = 3.14159;
float small = 1.5;

# String (double quotes)
String s = "hello";

# Character (single quotes)
char c = 'A';

# Boolean
boolean yes = true;
boolean no = false;

# null (reference type)
Ref<String> ref = null;
```

---

## Operator Precedence

Listed from highest to lowest precedence.

| Rank | Operators |
|------|--------|
| 1 | `**` (exponentiation) |
| 2 | `~` (bitwise NOT), unary `-` |
| 3 | `*`, `/`, `//`, `%` |
| 4 | `+`, `-` |
| 5 | `<<`, `>>` |
| 6 | `&` |
| 7 | `^` |
| 8 | `\|` |
| 9 | `==`, `!=`, `<`, `>`, `<=`, `>=`, `in`, `is` |
| 10 | `not` |
| 11 | `and` |
| 12 | `or` |

---
