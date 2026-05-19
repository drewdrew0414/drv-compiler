# Example 3: Ownership System + Arena Allocator

> Shows Own<T>/Ref<T>/Borrow<T> differences through practical patterns.
> Implements high-performance memory management without fragmentation using @region arenas.

---

## Overview

- `Own<T>` : exclusive ownership (original invalidated after move)
- `Ref<T>` : shared ownership (reference-counted)
- `Borrow<T>` : read-only temporary reference
- `mut Borrow<T>` : exclusive mutable reference
- `@region` : scope-based arena — bulk release at block exit

---

## Code

```dri
module arena_example;

# ── 1. Basic ownership ────────────────────────────────────────────

class Node {
    int value;
    String label;
}

void demo_ownership() {
    # Own<T>: exclusive ownership — cannot use original after move
    Own<Node> a = new Node();
    a.value = 42;
    a.label = "first";

    Own<Node> b = move a;   # transfer ownership
    print(`b.value = ${b.value}`);
    # a is now invalid — compiler detects this

    # Ref<T>: shared ownership — can be referenced from multiple places
    Ref<Node> r1 = new Node();
    r1.value = 100;
    Ref<Node> r2 = r1;      # reference count increases
    r2.value = 200;         # r1 and r2 share the same object
    print(`r1.value = ${r1.value}`);  # prints 200
}

# ── 2. Borrow — temporary read reference ──────────────────────────

double sum_values(Borrow<list<int>> arr) {
    # Borrow<T>: read-only, no ownership transfer
    double s = 0.0;
    for (x of arr) { s += x; }
    return s;
}

void scale_inplace(mut Borrow<list<double>> arr, double factor) {
    # mut Borrow<T>: exclusive mutable reference
    for (i in 0..lst.length(arr)) {
        arr[i] = arr[i] * factor;
    }
}

# ── 3. @region arena allocation ──────────────────────────────────

class Particle {
    double x;
    double y;
    double vx;
    double vy;
    double mass;
}

void simulate_frame(int n_particles) {
    @region frame {
        # all memory allocated inside this block is
        # bulk-released when the block exits (no heap fragmentation)
        list<Particle> particles = [];
        double[] forces_x = lst.fill(n_particles, 0.0);
        double[] forces_y = lst.fill(n_particles, 0.0);
        double[] temp_buf = lst.fill(n_particles * 4, 0.0);

        # initialise particles
        for (i in 0..n_particles) {
            Particle p;
            p.x = i * 0.1;
            p.y = 0.0;
            p.vx = 0.0;
            p.vy = 0.0;
            p.mass = 1.0;
            lst.push(particles, p);
        }

        # compute forces
        for (i in 0..n_particles) {
            forces_x[i] = -particles[i].x * 9.8;  # simple restoring force
            forces_y[i] = -9.8;
        }

        # update positions
        for (i in 0..n_particles) {
            particles[i].vx += forces_x[i] * 0.016;
            particles[i].vy += forces_y[i] * 0.016;
            particles[i].x  += particles[i].vx * 0.016;
            particles[i].y  += particles[i].vy * 0.016;
        }

        print(`frame done: ${n_particles} particles processed`);
        # ← here particles, forces_x, forces_y, temp_buf are all auto-released
    }
    print("arena released");
}

# ── 4. Nested arenas ──────────────────────────────────────────────

void nested_arenas() {
    @region outer {
        double[] persistent = lst.fill(1000, 1.0);

        for (frame in 0..60) {
            @region inner {
                # per-frame temporary data — reused every frame
                double[] scratch = lst.fill(10000, 0.0);
                double[] indices = lst.fill(5000, 0.0);

                # processing logic...
                for (i in 0..1000) {
                    scratch[i] = persistent[i] * frame;
                }
                # inner exits: scratch, indices released
            }
            # persistent stays alive in outer scope
        }
        # outer exits: persistent released
    }
}

# ── 5. Escape analysis example ────────────────────────────────────

# @region escape analysis: prevents pointers from leaking outside the arena
void escape_analysis_demo() {
    String leaked_ref;  # declared outside @region

    @region temp {
        String local_str = "string inside the arena";
        # compiler warning: local_str escapes outside the @region
        # leaked_ref = local_str;  ← this line is a compile error
        print(local_str);  # OK: used inside the arena
    }
    # using leaked_ref here would cause use-after-free — dri blocks this
}

# run
demo_ownership();

list<int> nums = [1, 2, 3, 4, 5];
print(`sum: ${sum_values(nums)}`);

list<double> vals = [1.0, 2.0, 3.0];
scale_inplace(vals, 2.0);
for (v of vals) { print(v); }

simulate_frame(1000);
nested_arenas();
escape_analysis_demo();
```

---

## Ownership Rules Summary

```
Own<T>  ──→  move  ──→  Own<T>     # move only, no copy
Ref<T>  ──→  share ──→  Ref<T>     # reference count
Borrow<T>        ←── read-only temporary reference
mut Borrow<T>    ←── exclusive mutable reference (not concurrent)
```

| Type | C++ mapping | Cost | Use case |
|------|-------------|------|---------|
| `Own<T>` | `unique_ptr<T>` | zero-cost | Sole owner |
| `Ref<T>` | `shared_ptr<T>` | ref-count | Shared ownership |
| `Borrow<T>` | `const T&` | zero-cost | Read-only |
| `mut Borrow<T>` | `T&` | zero-cost | Mutable reference |

---

## Key Takeaways

- Attempting to copy `Own<T>` without `move` → compile error
- A pointer from inside `@region` escaping outside → compile error
- Only one `mut Borrow<T>` can exist at a time (prevents data races)
