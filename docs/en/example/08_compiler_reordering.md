# Example 8: Compiler Reordering & CPU Pipeline Optimization

> Observe and optimize pipeline stalls, branch misprediction, loop unrolling,
> and memory barriers using dri code.

---

## Overview

Modern CPUs are complex pipelined machines, not just "fast processors."  
This example covers four ways **code structure affects hardware performance**.

| Optimization | Problem | Solution |
|---|---|---|
| Pipeline stall prevention | `result = result + data[i]` — RAW hazard | 4-way independent accumulators |
| Branch prediction | Random true/false → ~50% misprediction | Sort data before loop |
| Loop unrolling + SIMD | Per-iteration branch overhead | `simd for` + `step 4` |
| Memory barrier | Compiler/CPU reorders writes | `sys.fence_full()` |

---

## Key Concepts

### ① Pipeline Stalls (RAW Hazard)

```dri
# Slow: every iteration waits for the previous result → pipeline stall
@bench
double sum_serial(double[] data) {
    double result = 0.0;
    for (i in 0..lst.length(data)) {
        result = result + data[i];
    }
    return result;
}

# Fast: 4 independent accumulators → superscalar parallel execution
@bench
double sum_4accum(double[] data) {
    double s0 = 0.0; double s1 = 0.0;
    double s2 = 0.0; double s3 = 0.0;
    # ... (step 4 loop updates all four simultaneously)
    return s0 + s1 + s2 + s3;
}
```

**Why it works**: `s0·s1·s2·s3` are independent — the CPU's out-of-order  
engine executes all four simultaneously → ~3–4× throughput improvement.

### ② Branch Prediction

```dri
# Unsorted: positive/negative values scattered → ~50% misprediction rate
int count_positive_unsorted(double[] data) { ... }

# Sorted: positives cluster at the front → ~99% prediction accuracy
int count_positive_sorted(double[] data) {
    double[] sorted = lst.sort(data);
    ...
}
```

### ③ Loop Unrolling + SIMD

```dri
# SIMD hint: auto-vectorize to process 4–8 elements per cycle
void zero_simd(mut Borrow<double[]> arr) {
    simd for (i in 0..lst.length(arr)) { arr[i] = 0.0; }
}

# Manual unroll: step 4 removes loop overhead
void zero_unrolled(mut Borrow<double[]> arr) {
    for (i in 0..n4 step 4) {
        arr[i] = 0.0; arr[i+1] = 0.0; arr[i+2] = 0.0; arr[i+3] = 0.0;
    }
}
```

### ④ Memory Barriers

```dri
void safe_producer(mut Borrow<int[]> shared, int val) {
    shared[0] = val;
    sys.fence_full();   # ensure data is visible before the ready flag
    shared[1] = 1;      # ready flag
}
```

---

## Source

Full source: [`example08.dri`](example08.dri)

---

## Syntax Reference

| Syntax | Meaning |
|--------|---------|
| `@bench` | Print function wall-clock time to stderr |
| `simd for (i in ...)` | OpenMP `simd` vectorization hint |
| `for (i in 0..n step 4)` | `i += 4` increment (manual unroll) |
| `sys.fence_full()` | `std::atomic_thread_fence(seq_cst)` |
| `mut Borrow<T[]>` | `std::vector<T>&` — in-place modification |
