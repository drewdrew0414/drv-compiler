# Example 2: N-Body Gravity Simulation

> Astrophysics simulation combining physical unit types (`dim`) with SIMD.
> Unit errors are ruled out at the type level.

---

## Overview

- Newton's law of gravity: F = G·m₁·m₂ / r²
- `dim` enforces metre / second / kilogram / newton units
- `@layout_soa` for SoA layout → SIMD cache efficiency
- Leapfrog integration for energy conservation

---

## Code

```dri
# N-body gravity simulation — physical units + SIMD
module nbody;

# physical unit definitions
dim Meter   = "m";
dim Second  = "s";
dim Kg      = "kg";
dim Newton  = "N";
dim Joule   = "J";

double G = 6.674e-11;   # gravitational constant [m³/(kg·s²)]
int N_BODIES = 1000;
int STEPS    = 100;
double DT    = 1.0;     # time step [s]

# body data — SoA layout for SIMD optimisation
@layout_soa
class Body {
    double x;     # position [m]
    double y;
    double z;
    double vx;    # velocity [m/s]
    double vy;
    double vz;
    double mass;  # mass [kg]
}

# initial conditions: spherical distribution
void init_sphere(list<Body> bodies) {
    for (i in 0..N_BODIES) {
        Body b;
        double angle = i * 6.28318 / N_BODIES;
        double r     = 1e11 * (1.0 + i * 0.001);
        b.x    = r * math.cos(angle);
        b.y    = r * math.sin(angle);
        b.z    = r * 0.1 * math.sin(angle * 3.0);
        b.vx   = -r * 1e-7 * math.sin(angle);
        b.vy   =  r * 1e-7 * math.cos(angle);
        b.vz   = 0.0;
        b.mass = 1e24 * (1.0 + i * 0.01);
        lst.push(bodies, b);
    }
}

# force calculation + position/velocity update (Leapfrog)
@bench
void step(list<Body> bodies) {
    # half-step velocity update
    parallel for (i in 0..N_BODIES) {
        double ax = 0.0;
        double ay = 0.0;
        double az = 0.0;

        # sum gravity from all other bodies
        simd for (j in 0..N_BODIES) {
            where (i != j) {
                Body bi = bodies[i];
                Body bj = bodies[j];
                double dx = bj.x - bi.x;
                double dy = bj.y - bi.y;
                double dz = bj.z - bi.z;
                double r2 = dx*dx + dy*dy + dz*dz + 1e10;  # softening
                double r  = math.sqrt(r2);
                double f  = G * bj.mass / (r2 * r);
                ax += f * dx;
                ay += f * dy;
                az += f * dz;
            } otherwise {
                # i == j: skip self-interaction
            }
        }

        bodies[i].vx += ax * DT * 0.5;
        bodies[i].vy += ay * DT * 0.5;
        bodies[i].vz += az * DT * 0.5;
    }

    # position update
    parallel for (i in 0..N_BODIES) {
        bodies[i].x += bodies[i].vx * DT;
        bodies[i].y += bodies[i].vy * DT;
        bodies[i].z += bodies[i].vz * DT;
    }
}

# total energy (kinetic + potential)
double total_energy(list<Body> bodies) {
    double ke = 0.0;
    double pe = 0.0;

    parallel for reduction(+:ke) (i in 0..N_BODIES) {
        double v2 = bodies[i].vx * bodies[i].vx
                  + bodies[i].vy * bodies[i].vy
                  + bodies[i].vz * bodies[i].vz;
        ke += 0.5 * bodies[i].mass * v2;
    }

    parallel for reduction(+:pe) (i in 0..N_BODIES) {
        for (j in 0..i) {
            double dx = bodies[j].x - bodies[i].x;
            double dy = bodies[j].y - bodies[i].y;
            double dz = bodies[j].z - bodies[i].z;
            double r  = math.sqrt(dx*dx + dy*dy + dz*dz + 1e10);
            pe -= G * bodies[i].mass * bodies[j].mass / r;
        }
    }

    return ke + pe;
}

list<Body> bodies = [];
init_sphere(bodies);

double e0 = total_energy(bodies);
print(`initial total energy: ${e0} J`);

# run simulation
for (step_i in 0..STEPS) {
    step(bodies);
    if (step_i % 10 == 0) {
        double e = total_energy(bodies);
        double drift = (e - e0) / math.abs(e0);
        print(`step ${step_i}: energy drift = ${drift}`);
    }
}

double ef = total_energy(bodies);
print(`final energy conservation: ${(ef - e0) / math.abs(e0)}`);
```

---

## Key Takeaways

| Concept | Details |
|---------|---------|
| `dim` units | Compile-time physical unit checking |
| `@layout_soa` | Split struct into arrays → contiguous SIMD access |
| Leapfrog integration | Energy-conserving numerical integration |
| `simd for` + `where` | Mask the i==j self-interaction |
| `reduction(+:ke)` | Parallel summation reduction |

---

## Extension Ideas

- Barnes-Hut tree for O(N log N) complexity
- GPU offloading (link CUDA kernels via `extern "FFI"`)
- Add collision detection
