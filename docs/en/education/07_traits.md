# Traits and impl
> A `trait` defines a set of methods a type must implement. `impl` provides a trait's implementation for a specific class.

---

## trait — defining an interface

```
trait Name {
    return_type method_name(parameters);
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

## impl — implementing a trait

```
impl TraitName for ClassName {
    return_type method_name(parameters) {
        body
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

## Implementing multiple traits

A single class can implement multiple traits independently.

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

## The `Self` keyword

Within a trait method, `Self` refers to the implementing type itself.

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

## Trait bounds — generic type constraints

Declares a generic function that accepts only types implementing a particular trait.
It plays the same role as C++20 Concepts and Rust Trait Bounds.

```
return_type function_name<T implements TraitName>(T parameter) { body }
```

```dri
trait Printable {
    void print_info();
};

trait Comparable {
    boolean less_than(Self other);
};

# T must implement Printable — otherwise it is a compile error
void display<T implements Printable>(T item) {
    item.print_info();
};
```

Requiring multiple traits simultaneously:

```
T implements TraitA + TraitB
```

```dri
# T must implement both Printable and Comparable
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

Also applicable to return types:

```dri
T max_of<T implements Comparable>(T a, T b) {
    if (b.less_than(a)) {
        return a;
    };
    return b;
};
```

### Unbounded generics vs bounded generics

```dri
# Unbounded — cannot call methods on T (useful only for things like identity)
T identity<T>(T x) {
    return x;
};

# Bounded — can call the trait methods of T
void show<T implements Printable>(T x) {
    x.print_info();   # OK because Printable is guaranteed
};
```

### Compiler error message specification

```dri
class Foo { public int x; };  # Does not implement Printable

display(Foo());  # Compile error:
# error: 'Foo' does not implement trait 'Printable'
#        required by function 'display<T implements Printable>'
```

---

## Full example — shape computation system

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
print(c.name(), "area:", c.area(), "perimeter:", c.perimeter());

Rect r = new Rect();
r.w = 4.0;
r.h = 5.0;
print(r.name(), "area:", r.area(), "perimeter:", r.perimeter());
```

> For classes and inheritance concepts, see [[06_functions_classes.md](06_functions_classes.md)].

---
