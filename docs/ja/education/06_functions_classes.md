# 関数とクラス

---

## 関数の基本宣言
```dri
修飾子 戻り値の型 関数名(型 引数, ...) {
    本体
};
```
### 修飾子
| 修飾子 | 説明 |
| --- |------------------------------------------------------------------|
| public | すべてのファイルから使用できるようにします。 |
| private | 該当ファイル内でのみ使用できるようにします。 |
| static | オブジェクト (インスタンス) を生成せずに、クラス名.メソッド名() の形式で直接呼び出せるようにします。 |
| abstract | 抽象メソッドを作るときに使います。本体 `{}` を持たず宣言のみで、継承した子クラスに必ず実装させます。 |
| final | オーバーライド (再定義) を禁止します。子クラスでこのメソッドを変更できないようにします。 |
| synchronized | マルチスレッド環境での同期を保証します。あるスレッドがこのメソッドを実行している間、他のスレッドは待機します。 |

### 戻り値の型
すべての型が使用可能です [ [型を見る](02_basics.md) ]

### 戻り値
```dri
# 単一の戻り値
int max_of(int a, int b) {
    if (a > b) {
        return a;
    };
    return b;
};

# 早期 return
int safe_divide(int a, int b) {
    if (b == 0) {
        return 0;
    };
    return a / b;
};

print(max_of(10, 25));       # 25
print(safe_divide(10, 0));   # 0
print(safe_divide(10, 3));   # 3
```

---

## void 関数

```dri
void print_stars(int n) {
    for (i in 0..n) {
        print("*");
    };
};

void print_header(String title) {
    print("===", title, "===");
};

print_header("結果");
print_stars(10);
```

---

## 引数のデフォルト値

```dri
int power(int base, int exp = 2) {
    int result = 1;
    for (i in 0..exp) {
        result *= base;
    };
    return result;
};

print(power(3));      # 9 (デフォルト値 exp=2)
print(power(3, 3));   # 27
```

---

## 再帰関数

```dri
int factorial(int n) {
    if (n <= 1) {
        return 1;
    };
    return n * factorial(n - 1);
};

int fib(int n) {
    if (n <= 1) {
        return n;
    };
    return fib(n - 1) + fib(n - 2);
};

print(factorial(5));   # 120
print(fib(10));        # 55
```

---

## アノテーション付きの関数

```dri
@inline
int square(int x) {
    return x * x;
};

@pure
double normalize(double x, double lo, double hi) {
    return (x - lo) / (hi - lo);
};

@fastcall
void hot_path(int a, int b) {
    print(a + b);
};
```

---

## async 関数

```dri
async void fetch_data(String path) {
    String content = await io.read_file(path);
    print("受信したデータ:", length(content), "バイト");
};

fetch_data("data.txt");
```

---

## ジェネリック関数

```dri
# 戻り値の型が T のジェネリック関数
T identity<T>(T x) {
    return x;
};
```

---

## クラス

### 基本クラス

```
class 名前 {
    public 型 フィールド名;
    public 戻り値の型 メソッド名(引数) {
        本体
    };
};
```

> クラス宣言全体は `;` で終わります。フィールドやメソッドも各々 `;` で終わります。

```dri
class Point {
    public int x;
    public int y;

    public void move(int dx, int dy) {
        this.x += dx;
        this.y += dy;
    };

    public String to_string() {
        return "(" + str.from_int(this.x) + ", " + str.from_int(this.y) + ")";
    };
};
```

### インスタンスの生成

```dri
Point p = new Point();
p.x = 3;
p.y = 7;

p.move(2, -1);
print(p.to_string());    # (5, 6)
```

---

## フィールド宣言

```dri
class Config {
    public String host;
    public int port;
    public boolean tls;
    public double timeout;
};

Config cfg = new Config();
cfg.host = "localhost";
cfg.port = 8080;
cfg.tls = false;
cfg.timeout = 30.0;
```

複数のフィールドを 1 行で宣言:

```dri
class Vec3 {
    public float x, y, z;   # x, y, z を同時宣言
};
```

---

## `this` キーワード

クラスメソッド内で現在のインスタンスのフィールドにアクセスする際に使用します。

```dri
class Counter {
    public int value;

    public void increment() {
        this.value += 1;
    };

    public void add(int n) {
        this.value += n;
    };

    public int get() {
        return this.value;
    };
};

Counter c = new Counter();
c.value = 0;
c.increment();
c.increment();
c.add(5);
print(c.get());   # 7
```

---

## 継承 — `extends`

```dri
class Animal {
    public String name;
    public int age;

    public void describe() {
        print("名前:", this.name, "年齢:", this.age);
    };

    public void speak() {
        print("...");
    };
};

class Dog extends Animal {
    public String breed;

    override void speak() {
        print("ワンワン!");
    };

    public void fetch() {
        print(this.name, "がボールを持ってきます。");
    };
};

class Cat extends Animal {
    override void speak() {
        print("ニャー~");
    };
};
```

```dri
Dog d = new Dog();
d.name = "バドゥギ";
d.age = 3;
d.breed = "珍島犬";
d.describe();    # 名前: バドゥギ 年齢: 3
d.speak();       # ワンワン!
d.fetch();       # バドゥギ がボールを持ってきます。

Cat c = new Cat();
c.name = "ナビ";
c.age = 2;
c.speak();       # ニャー~
```

---

## 抽象クラス — `abstract class`

インスタンス化できず、子クラスが必ず実装しなければなりません。
抽象メソッドには `@abstract` アノテーションを使用します。

```dri
abstract class Shape {
    public String color;

    @abstract double area();
    @abstract double perimeter();

    public void describe() {
        print("図形:", this.color,
              "面積:", this.area(),
              "周長:", this.perimeter());
    };
};

class Circle extends Shape {
    public double radius;

    override double area() {
        return 3.14159 * this.radius * this.radius;
    };

    override double perimeter() {
        return 2.0 * 3.14159 * this.radius;
    };
};

class Rectangle extends Shape {
    public double width;
    public double height;

    override double area() {
        return this.width * this.height;
    };

    override double perimeter() {
        return 2.0 * (this.width + this.height);
    };
};
```

```dri
Circle circ = new Circle();
circ.color = "赤";
circ.radius = 5.0;
circ.describe();

Rectangle rect = new Rectangle();
rect.color = "青";
rect.width = 4.0;
rect.height = 6.0;
rect.describe();
```

---

## `@packed` — パディングなしのクラス

フィールド間のパディングを取り除いてメモリを節約します。

```dri
@packed
class Pixel {
    public char r;
    public char g;
    public char b;
    public char a;    # 4 バイト密着、パディングなし
};
```

> **制約**: `@packed` クラスは **継承に関与できません。**
> 継承されたクラスは vtable ポインタを含み、`@packed` はそのポインタのアラインメントを壊し、
> CPU レベルの Misaligned Access やクラッシュを引き起こします。
> コンパイラは以下のいずれの場合も **コンパイルエラー** を発生させます。

```dri
# エラー: @packed クラスは他のクラスを継承できません
@packed
class PackedCircle extends Shape { ... };   # コンパイルエラー

# エラー: @packed クラスを継承できません
class Child extends Pixel { ... };          # コンパイルエラー
```

`@packed` は **最終リーフ (leaf) データ構造** — つまり継承のない純粋なデータクラスにのみ使用してください。

---

## フィールドチェーンアクセス

```dri
class Engine {
    public int horsepower;
};

class Car {
    public String name;
    public Engine engine;
};

Car car = new Car();
car.name = "Turbo";
car.engine = new Engine();
car.engine.horsepower = 300;

print(car.engine.horsepower);    # 300
car.engine.horsepower = 350;     # チェーン代入
```

---

## null チェック

```dri
Ref<Point> p = null;

if (p != null) {
    print(p.to_string());
} else {
    print("ポイントがありません");
};

p = new Point();
p.x = 10;
p.y = 20;
print(p.to_string());   # (10, 20)
```

---
