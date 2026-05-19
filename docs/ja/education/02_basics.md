# 基本文法 — コメント、変数、型、入出力
> すべての文はセミコロンで終わります。

---

## コメント
```dri
# コメントは "#" を用いて記述します。
!# 複数行コメントを書きたい場合は、
   "!#" で始めて、
   "##" で終わります。
##
```

---

## 変数宣言
```dri
int age = 25;
double pi = 3.14159;
String name = "Alice";
boolean flag = true;
char grade = 'A';
long population = 8000000000;
```

### 型推論 — ```var```
```dri
var x = 42;        # intと推論
var y = 3.14;      # doubleと推論
var msg = "hello"; # Stringと推論
```

### `var`の許容範囲

| 位置 | `var`の可否 | 理由 |
|------|----------------|------|
| ローカル変数 | 可 | 右辺の式から型を確定できる |
| forループ変数 | 可 | コレクションの要素型から型を確定できる |
| **関数の戻り値型** | 不可 | コンパイラが呼び出し側で型検査できない |
| **関数の仮引数** | 不可 | オーバーロード・ジェネリック解決が不可、トランスパイル負荷が急増 |
| **クラスフィールド** | 不可 | 構造体レイアウトをコンパイル時に確定できない |

```dri
# 許可
var result = compute();           # ローカル変数
for (var item of list) { ... };   # ループ変数

# コンパイルエラー
var add(var a, var b) { ... };    # エラー: 仮引数・戻り値型にvarは不可

class Foo {
    var value;                     # エラー: フィールドにvarは不可
};
```

> HPC言語という性質上、関数シグネチャは**必ず型を明示**しなければなりません。
> これによりC++トランスパイラがABIとオーバーロードを明確に決定できます。

---

## 型
### 基本型
| 型 | サイズ | 例 |
|------|------------|------|
| `int` | 32ビット整数 | `int n = 0;` |
| `long` | 64ビット整数 | `long n = 0;` |
| `float` | 32ビット浮動小数点 | `float f = 0.0;` |
| `double` | 64ビット浮動小数点 | `double d = 0.0;` |
| `char` | 8ビット文字 | `char c = 'A';` |
| `String` | 可変文字列 | `String s = "";` |
| `boolean` | 8ビット真偽値 | `boolean b = true;` |
| `void` | — | 関数の戻り値型のみ |

### 特殊型
| 型 | 格納方法 | 例 |
| --- |-----| --- |
| `arg` | 複数のString > list | `arg a = "h", "e", "l", "l", "o"` |

### コレクション型
| 型 | C++対応                    | 説明 | 例                              |
| --- |---------------------------| --- |---------------------------------|
| `array` / `int[]` | `std::vector<T>`          | 動的配列 | [例を見る](#array-/-int[]-사용-예시) |
| `list<T>` | `	std::vector<T>`         | ジェネリックリスト | [例を見る](#list<T>-사용-예시)       |
| `Map<K, V>` | `std::unordered_map<K,V>` | ハッシュマップ | [例を見る](#Map<K,V>-사용-예시)       |
| `tensor` | `std::array / SIMD配列`    | 数値計算配列 | [例を見る](#tensor-사용-예시)       |
| `tensor<N,T>`	| `std::array<T,N>`           |	静的サイズのtensor | [例を見る](#tensor<N,T>-사용-예시)       |

### 参照型
### `Ref<T>` — 共有所有権(`shared_ptr`)
```dri
Ref<String> s = new String();
Ref<MyClass> obj = new MyClass();
Ref<int[]> buf = new int[100];

# null許容
Ref<String> maybe = null;
if (maybe != null) {
    print(maybe);
};
```

### `Own<T>` — 単一所有権(`unique_ptr`)

```dri
Own<int[]> buf = new int[1024];
Own<int[]> other = move buf;  # 所有権移転後はbufが使用不可
```

### `Borrow<T>` — 非所有参照(`const T&`)

```dri
void read_only(Borrow<int[]> data) {
    print(data[0]);
};
```

## `Option<T>` — あるか、ないか

```dri
Option<int> safe_div(int a, int b) {
    if (b == 0) {
        return None;
    };
    return Some(a / b);
};

match safe_div(10, 2) {
    Some(v) => { print("結果:", v); };
    None    => { print("除算失敗"); };
};
```

## `Result<T>` — 成功または失敗

```dri
Result<int> parse_num(String s) {
    if (length(s) == 0) {
        return Err("空文字列");
    };
    return Ok(str.to_int(s));
};

match parse_num("42") {
    Ok(v)  => { print("パース成功:", v); };
    Err(e) => { print("エラー:", e); };
};
```

## `dim` — 物理単位型

```dri
dim Meter = "m";
dim Second = "s";

Meter dist = 5.0;
Second time = 2.0;
```

### 単位演算ルール

単位同士の四則演算の結果型は以下のルールに従います。

| 演算 | 条件 | 結果の型 |
|------|------|----------|
| `A + B`, `A - B` | 同一単位 | 同一単位 |
| `A + B`, `A - B` | 異なる単位 | **コンパイルエラー** |
| `A * B` | 任意の単位 | 複合単位(新しい`dim`の定義が必要) |
| `A / B` | 任意の単位 | 複合単位(新しい`dim`の定義が必要)または`double` |
| `A * scalar` | — | 同一単位 |
| `A / scalar` | — | 同一単位 |

```dri
dim Meter = "m";
dim Second = "s";
dim MeterPerSecond = "m/s";   # 複合単位を事前定義

Meter dist = 10.0;
Second time = 2.0;

# 同一単位の加算 — OK
Meter total = dist + Meter(3.0);   # 13.0 m

# 異なる単位の加算 — コンパイルエラー
# Meter wrong = dist + time;       # エラー: Meter + Secondは不可

# 除算 — 複合単位として受けるか、doubleとして受ける
MeterPerSecond speed = dist / time;   # 5.0 m/s(複合単位定義がある場合)
double ratio = dist / time;           # 5.0(単位を切り捨てる。doubleで受ければ許容)

# スカラー乗除算
Meter half = dist * 0.5;    # 5.0 m
Meter scaled = dist / 2.0;  # 5.0 m
```

未定義の複合単位の結果:

```dri
# 複合単位(dim)を定義せずに除算した結果 → doubleでのみ受けられる
dim Kg = "kg";
Meter dist2 = 3.0;
Kg mass = 5.0;

double ratio2 = dist2 / mass;   # OK: 単位を切り捨ててdoubleで受ける
# Meter kg_ratio = dist2 / mass;  # エラー: 未定義の複合単位
```

## `Union` 型

```dri
Union<int, String> val = 42;
Union<int, String> val2 = "hello";
```

## `ref` — 参照変数(`T&`)

```dri
int x = 10;
ref int y = x;   # yはxの参照
y = 20;          # xも20になる
print(x);        # 20
```

## リフレクション

```dri
int x = 42;
String t = reflect.type_of(x);   # "int"
print(t);
```

---

## 修飾子
```dri
const int MAX = 100;      # 定数
unsigned int mask = 0xFF; # 符号なし整数
mut int counter = 0;      # 可変を明示(既定でも可変)
```

| 修飾子 | 説明 |
| ----- | --- |
| const | 定数 |
| unsigned | 符号なし整数 |
| mut | 可変を明示(既定でも可変) |

---

## 入出力
### 出力
```dri
print("Hello World!");          # >>> Hello World!
print(42);                      # >>> 42
print(true);                    # >>> true

int x = 10;                     # >>> x = 10 を定義
String name = "dri";            # >>> name = "dri" を定義
print(name, " version: ", x);   # >>> dri version: 10
print(name + " version: " + x); # >>> dri version: 10
print(`{name} version: {x}`);   # >>> dri version: 10
print(name * 3);                # >>> dridridri
```

### 入力
```dri
String usrInput; # 左記のように定義するとnullが既定値です。
input("値を入力してください >>> ", usrInput);
usrInput = input("値を入力してください >>> ");
```
> `<変数> = input(...);`とする場合、
> 形式は`input(<String>, <args>);`です。
