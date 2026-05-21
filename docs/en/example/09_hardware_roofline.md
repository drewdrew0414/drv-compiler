# Example 9: Hardware Peak Performance — Roofline Model

> Measure STREAM bandwidth, sweep the cache hierarchy, and visualize
> the Roofline model using dri code to observe hardware limits directly.

---

## Overview

Every algorithm hits one of two hardware ceilings:

```
Performance [GFLOP/s] = min(Peak_FLOPS,  AI × Peak_BW)
                                ↑            ↑       ↑
                          compute limit   intensity  memory limit
```

The **Roofline model** visualizes this relationship to diagnose whether
the bottleneck is computation or memory bandwidth.

| Term | Definition |
|------|-----------|
| Arithmetic Intensity (AI) | FLOPs ÷ bytes accessed |
| Memory-Bound | AI < Ridge Point → memory bandwidth is the bottleneck |
| Compute-Bound | AI > Ridge Point → compute throughput is the bottleneck |
| Ridge Point | Peak_FLOPS ÷ Peak_BW [FLOP/Byte] |

---

## Key Concepts

### STREAM Benchmark

```dri
@bench
void stream_triad(Borrow<double[]> a, Borrow<double[]> b,
                  mut Borrow<double[]> c, double scalar) {
    for (i in 0..lst.length(c)) {
        c[i] = a[i] + scalar * b[i];
    }
}
```

STREAM Triad is the standard memory bandwidth indicator.  
It accesses `3N × 8 bytes`, giving `AI ≈ 1/12 FLOP/Byte`.

### Cache Hierarchy Sweep

```dri
# Small array (fits in L1): hundreds of GB/s
# Medium array (L2/L3): tens of GB/s
# Large array (DRAM): ~10–20 GB/s
```

### Text Roofline Visualization

```
▲ GFLOP/s
│    ╔══════════════════ Peak FLOPS
│    ╟─────────────────
│   /  compute-bound
│  /
│ / ← slope = Peak BW
│/___________________________▶ AI [FLOP/Byte]
   Ridge Point
```

---

## Source

Full source: [`example09.dri`](example09.dri)

---

## Syntax Reference

| Syntax | Meaning |
|--------|---------|
| `Borrow<double[]>` | `const std::vector<double>&` |
| `mut Borrow<double[]>` | `std::vector<double>&` |
| `@bench` | Per-function wall-clock timing |
| `parallel for` | OpenMP parallel loop |
| `sys.time_ms()` | Current time in milliseconds (high resolution) |
| `lst.fill(N, 0.0)` | Create list of size N filled with 0.0 |
