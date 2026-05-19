# トレイトと impl
> `trait` は、型が必ず実装しなければならないメソッドの集合を定義します。`impl` は特定のクラスにトレイトを実装します。

---

## trait — インターフェースの定義

```
trait 名前 {
    戻り値の型 メソッド名(引数);
};
```

```dri
trait Printable {
    void print_info();
};

trait Comparable {
    boolean equals(Self other);
    boolean less_than(Self other);
};

trait Serializable {
    String to_json();
};
```

---

## impl — トレイトの実装

```
impl トレイト名 for クラス名 {
    戻り値の型 メソッド名(引数) {
        本体
    };
};
```

```dri
class Point {
    public int x;
    public int y;
};

impl Printable for Point {
    void print_info() {
        print("Point(", this.x, ",", this.y, ")");
    };
};

impl Comparable for Point {
    boolean equals(Self other) {
        return this.x == other.x and this.y == other.y;
    };

    boolean less_than(Self other) {
        return this.x < other.x or (this.x == other.x and this.y < other.y);
    };
};
```

```dri
Point p1 = new Point();
p1.x = 3;
p1.y = 4;

Point p2 = new Point();
p2.x = 1;
p2.y = 2;

p1.print_info();               # Point( 3 , 4 )
print(p1.equals(p2));          # false
print(p2.less_than(p1));       # true
```

---

## 複数のトレイトの実装

1 つのクラスに複数のトレイトを独立して実装できます。

```dri
trait Area {
    double area();
};

trait Perimeter {
    double perimeter();
};

trait Describe {
    void describe();
};

class Circle {
    public double radius;
};

impl Area for Circle {
    double area() {
        return 3.14159 * this.radius * this.radius;
    };
};

impl Perimeter for Circle {
    double perimeter() {
        return 2.0 * 3.14159 * this.radius;
    };
};

impl Describe for Circle {
    void describe() {
        print("Circle r =", this.radius,
              "area =", this.area(),
              "perimeter =", this.perimeter());
    };
};

Circle c = new Circle();
c.radius = 5.0;
c.describe();
```

---

## `Self` キーワード

トレイトメソッド内で、実装する型自身を指します。

```dri
trait Cloneable {
    Self clone();
};

class Config {
    public String host;
    public int port;
};

impl Cloneable for Config {
    Self clone() {
        Config c = new Config();
        c.host = this.host;
        c.port = this.port;
        return c;
    };
};

Config original = new Config();
original.host = "localhost";
original.port = 8080;

Config copy = original.clone();
copy.port = 9090;

print(original.port);   # 8080
print(copy.port);       # 9090
```

---

## トレイト境界 — ジェネリック型の制約

特定のトレイトを実装した型のみを引数として受け取るジェネリック関数を宣言します。
C++20 Concept や Rust Trait Bound と同じ役割を果たします。

```
戻り値の型 関数名<T implements トレイト名>(T 引数) { 本体 }
```

```dri
trait Printable {
    void print_info();
};

trait Comparable {
    boolean less_than(Self other);
};

# T は必ず Printable を実装しなければならない — そうでなければコンパイルエラー
void display<T implements Printable>(T item) {
    item.print_info();
};
```

複数のトレイトを同時に要求:

```
T implements トレイトA + トレイトB
```

```dri
# T は Printable と Comparable の両方を実装しなければならない
void print_sorted<T implements Printable + Comparable>(T a, T b) {
    if (a.less_than(b)) {
        a.print_info();
        b.print_info();
    } else {
        b.print_info();
        a.print_info();
    };
};
```

戻り値の型にも適用可能:

```dri
T max_of<T implements Comparable>(T a, T b) {
    if (b.less_than(a)) {
        return a;
    };
    return b;
};
```

### 制約なしのジェネリック vs 境界付きジェネリックの比較

```dri
# 制約なし — T に対するメソッド呼び出し不可 (identity 関数などにのみ有用)
T identity<T>(T x) {
    return x;
};

# 境界あり — T のトレイトメソッドを呼び出し可能
void show<T implements Printable>(T x) {
    x.print_info();   # Printable が保証されるので OK
};
```

### コンパイラエラーメッセージの規定

```dri
class Foo { public int x; };  # Printable 未実装

display(Foo());  # コンパイルエラー:
# error: 'Foo' does not implement trait 'Printable'
#        required by function 'display<T implements Printable>'
```

---

## 完全な例 — 図形計算システム

```dri
trait Shape {
    double area();
    double perimeter();
    String name();
};

class Circle {
    public double r;
};

impl Shape for Circle {
    double area() {
        return 3.14159 * this.r * this.r;
    };
    double perimeter() {
        return 2.0 * 3.14159 * this.r;
    };
    String name() {
        return "Circle";
    };
};

class Rect {
    public double w;
    public double h;
};

impl Shape for Rect {
    double area() {
        return this.w * this.h;
    };
    double perimeter() {
        return 2.0 * (this.w + this.h);
    };
    String name() {
        return "Rectangle";
    };
};

Circle c = new Circle();
c.r = 3.0;
print(c.name(), "面積:", c.area(), "周長:", c.perimeter());

Rect r = new Rect();
r.w = 4.0;
r.h = 5.0;
print(r.name(), "面積:", r.area(), "周長:", r.perimeter());
```

> クラス・継承の概念は [[06_functions_classes.md](06_functions_classes.md)] を参照してください。

---
