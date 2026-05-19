# 演算子

---

## 算術演算子

```dri
int a = 10;
int b = 3;

print(a + b);    # 13
print(a - b);    # 7
print(a * b);    # 30
print(a / b);    # 3.33333...
print(a // b);   # 3 (整数除算)
print(a % b);    # 1  (剰余)
print(a ** b);   # 1000 (累乗)

double x = 10.0;
double y = 3.0;
print(x / y);
```
> `//`演算子で変数に代入する場合は、float型で受ける必要があります(型エラー回避のため)。

---

## 複合代入演算子

```dri
int n = 10;
n += 5;    # n = 15
n -= 3;    # n = 12
n *= 2;    # n = 24
n /= 4;    # n = 6
n %= 4;    # n = 2
```
> `++`、`--`も使用できます。

---

## 比較演算子

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

## 論理演算子

`and`、`or`、`not`が正式なキーワードです。`&&`、`||`、`!`は後方互換のエイリアスのため、新しいコードでは使用を避けてください。

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

> `&&`、`||`、`!`はパーサが認識しますが、**非推奨(deprecated)**のエイリアスです。
> ビット演算子`&`、`|`と視覚的に区別するため、キーワード形式を正式な文法として採用しました。

---

## ビット演算子

```dri
int a = 0b1100;   # 12
int b = 0b1010;   # 10

print(a & b);    # 0b1000 = 8  (AND)
print(a | b);    # 0b1110 = 14 (OR)
print(a ^ b);    # 0b0110 = 6  (XOR)
print(~a);       # ビット反転 (NOT)
print(a << 2);   # 0b110000 = 48 (左シフト)
print(a >> 1);   # 0b0110 = 6   (右シフト)
```

| 演算子 | 名前 | 説明 |
|--------|------|------|
| `&` | AND | 両方のビットが1なら1 |
| `\|` | OR | どちらかが1なら1 |
| `^` | XOR | 2つのビットが異なれば1 |
| `~` | NOT | ビット反転 |
| `<<` | 左シフト | 2^n倍の効果 |
| `>>` | 右シフト | 2^nで割る効果 |

---

## `in`演算子(メンバーシップテスト)

```dri
int[] arr = [1, 2, 3, 4, 5];
print(3 in arr);    # true
print(9 in arr);    # false
```

---

## `is`演算子

```dri
int x = 5;
print(x is 5);      # true (== と同じ)
print(x is not 3);  # true (!= と同じ)
```

---

## 型キャスト — `as`

```dri
int n = 42;
double d = n as double;     # 42.0
int back = d as int;        # 42 (小数部切り捨て)

long big = 9999999999;
int small = big as int;     # オーバーフロー注意
```

---

## リテラル種類

```dri
# 整数
int dec = 255;
int hex = 0xFF;       # 16進数 (255)
int bin = 0b11001100; # 2進数

# 浮動小数点
double pi = 3.14159;
float small = 1.5;

# 文字列(二重引用符)
String s = "hello";

# 文字(単一引用符)
char c = 'A';

# 真偽値
boolean yes = true;
boolean no = false;

# null(参照型)
Ref<String> ref = null;
```

---

## 演算子の優先順位

優先順位が高い順に並べています。

| 順位 | 演算子 |
|------|--------|
| 1 | `**`(累乗) |
| 2 | `~`(ビットNOT)、単項`-` |
| 3 | `*`、`/`、`//`、`%` |
| 4 | `+`、`-` |
| 5 | `<<`、`>>` |
| 6 | `&` |
| 7 | `^` |
| 8 | `\|` |
| 9 | `==`、`!=`、`<`、`>`、`<=`、`>=`、`in`、`is` |
| 10 | `not` |
| 11 | `and` |
| 12 | `or` |

---
