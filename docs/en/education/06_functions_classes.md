# Functions and classes

---

## Basic function declaration
```dri
modifier return_type function_name(type parameter, ...) {
    body
};
```
### Modifiers
| Modifier | Description |
| --- |------------------------------------------------------------------|
| public | Makes the symbol usable from every file. |
| private | Restricts use to the file in which it is declared. |
| static | Allows direct invocation as ClassName.methodName() without creating an instance. |
| abstract | Declares an abstract method. The declaration has no body `{}` and forces every subclass to implement it. |
| final | Forbids overriding. The method cannot be redefined in a subclass. |
| synchronized | Guarantees synchronization in multithreaded environments. While one thread is executing the method, other threads wait. |

### Return type
Every type is allowed [ [See types](02_basics.md) ]

### Return values
```dri
# Single return
int max_of(int a, int b) {
    if (a > b) {
        return a;
    };
    return b;
};

# Early return
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

## void functions

```dri
void print_stars(int n) {
    for (i in 0..n) {
        print("*");
    };
};

void print_header(String title) {
    print("===", title, "===");
};

print_header("Result");
print_stars(10);
```

---

## Default parameter values

```dri
int power(int base, int exp = 2) {
    int result = 1;
    for (i in 0..exp) {
        result *= base;
    };
    return result;
};

print(power(3));      # 9 (uses default exp=2)
print(power(3, 3));   # 27
```

---

## Recursive functions

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

## Functions with annotations

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

## async functions

```dri
async void fetch_data(String path) {
    String content = await io.read_file(path);
    print("Received data:", length(content), "bytes");
};

fetch_data("data.txt");
```

---

## Generic functions

```dri
# Generic function whose return type is T
T identity<T>(T x) {
    return x;
};
```

---

## Classes

### Basic class

```
class Name {
    public type field_name;
    public return_type method_name(parameters) {
        body
    };
};
```

> The entire class declaration ends with `;`. Each field and method also ends with `;`.

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

### Creating instances

```dri
Point p = new Point();
p.x = 3;
p.y = 7;

p.move(2, -1);
print(p.to_string());    # (5, 6)
```

---

## Field declarations

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

Declaring multiple fields on a single line:

```dri
class Vec3 {
    public float x, y, z;   # Declare x, y, z together
};
```

---

## The `this` keyword

Used inside class methods to access the fields of the current instance.

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

## Inheritance — `extends`

```dri
class Animal {
    public String name;
    public int age;

    public void describe() {
        print("Name:", this.name, "Age:", this.age);
    };

    public void speak() {
        print("...");
    };
};

class Dog extends Animal {
    public String breed;

    override void speak() {
        print("Woof!");
    };

    public void fetch() {
        print(this.name, "fetches the ball.");
    };
};

class Cat extends Animal {
    override void speak() {
        print("Meow~");
    };
};
```

```dri
Dog d = new Dog();
d.name = "Baduki";
d.age = 3;
d.breed = "Jindo";
d.describe();    # Name: Baduki Age: 3
d.speak();       # Woof!
d.fetch();       # Baduki fetches the ball.

Cat c = new Cat();
c.name = "Nabi";
c.age = 2;
c.speak();       # Meow~
```

---

## Abstract classes — `abstract class`

Cannot be instantiated, and every subclass must implement the abstract methods.
Abstract methods use the `@abstract` annotation.

```dri
abstract class Shape {
    public String color;

    @abstract double area();
    @abstract double perimeter();

    public void describe() {
        print("Shape:", this.color,
              "Area:", this.area(),
              "Perimeter:", this.perimeter());
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
circ.color = "Red";
circ.radius = 5.0;
circ.describe();

Rectangle rect = new Rectangle();
rect.color = "Blue";
rect.width = 4.0;
rect.height = 6.0;
rect.describe();
```

---

## `@packed` — class without padding

Eliminates padding between fields to save memory.

```dri
@packed
class Pixel {
    public char r;
    public char g;
    public char b;
    public char a;    # Tightly packed into 4 bytes, no padding
};
```

> **Restriction**: `@packed` classes **cannot participate in inheritance.**
> Inherited classes contain a vtable pointer, and `@packed` breaks the alignment of that pointer,
> which can cause CPU-level misaligned access or crashes.
> The compiler raises a **compile error** in both of the following cases.

```dri
# Error: a @packed class cannot inherit from another class
@packed
class PackedCircle extends Shape { ... };   # compile error

# Error: cannot inherit from a @packed class
class Child extends Pixel { ... };          # compile error
```

Use `@packed` only for **final leaf data structures** — pure data classes that are not part of any inheritance chain.

---

## Field chain access

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
car.engine.horsepower = 350;     # Chained assignment
```

---

## null checks

```dri
Ref<Point> p = null;

if (p != null) {
    print(p.to_string());
} else {
    print("No point");
};

p = new Point();
p.x = 10;
p.y = 20;
print(p.to_string());   # (10, 20)
```

---
