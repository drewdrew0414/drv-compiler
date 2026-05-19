# External Libraries
> The four core standard libraries in the dri ecosystem. Install with `drvpm install`; each ships as an independent module.

---

## Library Overview

| Package | Role |
|---------|------|
| `dri-pandas` | DataFrame and pipeline operations |
| `dri-toulmin` | Toulmin-argument-based rule engine |
| `dri-data` | Multi-format I/O and conversion |
| `dri-plot` | High-performance data visualization |

```bash
drvpm install dri-pandas
drvpm install dri-toulmin
drvpm install dri-data
drvpm install dri-plot
```

---

## Compatibility Matrix

### dri compiler version

| Package | Minimum dri | Recommended | Notes |
|---------|------------|-------------|-------|
| `dri-pandas` | `≥ 0.6.0` | `≥ 1.0.0` | Requires Apache Arrow FFI |
| `dri-toulmin` | `≥ 0.5.0` | `≥ 0.6.0` | Needs `@warrant`/`@trace` annotations |
| `dri-data` | `≥ 0.6.0` | `≥ 1.0.0` | Depends on `dri-pandas` |
| `dri-plot` | `≥ 0.6.0` | `≥ 1.0.0` | Depends on `dri-data` |

### C++ backend

| Package | clang | GCC | MSVC | Notes |
|---------|-------|-----|------|-------|
| `dri-pandas` | `≥ 14` | `≥ 12` | `≥ 19.30` | Optional AVX2 support |
| `dri-toulmin` | `≥ 12` | `≥ 11` | `≥ 19.28` | — |
| `dri-data` | `≥ 14` | `≥ 12` | `≥ 19.30` | Requires libpng, libjpeg |
| `dri-plot` | `≥ 14` | `≥ 12` | `≥ 19.30` | Optional OpenGL / WebGPU |

### Platform

| Package | Linux | macOS | Windows | Notes |
|---------|-------|-------|---------|-------|
| `dri-pandas` | ✅ | ✅ | ✅ | WSL2 recommended |
| `dri-toulmin` | ✅ | ✅ | ✅ | — |
| `dri-data` | ✅ | ✅ | ✅ | PNG export requires libpng |
| `dri-plot` | ✅ | ✅ | ✅ | GUI: X11 / AppKit / Win32 |

### Package dependency graph

```
dri-plot
  └── dri-data
        └── dri-pandas
              └── [Apache Arrow / Rust Polars — FFI]
dri-toulmin
  └── [standalone — no other package dependencies]
```

Adding only `dri-plot` to `drvpm.drii` will automatically install `dri-data` and `dri-pandas` as well.

---

## dri-pandas — DataFrame and Pipeline Operations

### Backend

Wraps Apache Arrow or Rust Polars core via `extern "FFI"`.  
Column-based memory layout (`@layout_soa`) and zero-copy slicing handle large datasets efficiently.

```dri
use dri_pandas;

# load CSV
DataFrame df = pandas.read_csv("sales.csv");

# query and transform
DataFrame result = df
    .query("revenue > 1000")
    .assign("profit", |row| -> row["revenue"] - row["cost"])
    .groupby("region")
    .mean("profit");

print(result);
```

### Core methods

| Method | Description |
|--------|-------------|
| `pandas.read_csv(path)` | CSV file → DataFrame |
| `pandas.read_json(path)` | JSON file → DataFrame |
| `df.query(expr)` | Filter rows by condition |
| `df.assign(col, lambda)` | Add a derived column |
| `df.groupby(col)` | Start a group aggregation |
| `df.mean(col)` | Column mean |
| `df.sum(col)` | Column sum |
| `df.get_column(name)` | Extract a single column → Series |
| `df.head(n)` | First n rows |
| `df.shape()` | (row count, column count) |
| `df.to_tensor()` | Numeric columns → `tensor` |

### Method chaining

```dri
use dri_pandas;

DataFrame df = pandas.read_csv("students.csv");

DataFrame top = df
    .query("score >= 90")
    .assign("grade", |row| -> "A")
    .groupby("class")
    .mean("score");

for (row of top.iter_rows()) {
    print(row["class"], "average:", row["score"]);
};
```

---

## dri-toulmin — Argument-Based Rule Engine

> Reference: [park-jun-woo/toulmin](https://github.com/park-jun-woo/toulmin)

### Overview

A rule evaluation engine based on the Toulmin model of argumentation.  
Rules are defined with `@warrant`, `@rebuttal`, and `@defeats` annotations. The engine loads
the rule graph at runtime and computes an acceptance score to reach a verdict.

### Acceptability algorithm

$$Acc(a) = \frac{w(a)}{1 + \sum_{b \in attackers(a)} Acc(b)}$$

- `w(a)` — base weight of rule a
- `attackers(a)` — rules that attack a via rebuttal/defeats
- Converges through iteration (default 10 rounds)

### Usage

```dri
use dri_toulmin;

# define rules with @warrant / @rebuttal / @defeats annotations
@warrant("is_approved")
boolean income_check(double income) {
    return income >= 3000000.0;
};

@warrant("is_approved")
boolean credit_check(int credit_score) {
    return credit_score >= 700;
};

@rebuttal("is_approved")
boolean has_debt(double debt_ratio) {
    return debt_ratio > 0.5;
};

@defeats("has_debt")
boolean collateral_exists(boolean has_collateral) {
    return has_collateral;
};

# initialise the engine — auto-loads rule graph from binary metadata
RuleEngine engine = toulmin.load_rules();

# input facts
Map<String, double> facts = {};
map.set(facts, "income", 3500000.0);
map.set(facts, "credit_score", 720.0);
map.set(facts, "debt_ratio", 0.6);
map.set(facts, "has_collateral", 1.0);

# evaluate
ToulminResult result = engine.evaluate("is_approved", facts);

print("Approved:", result.accepted);       # true
print("Acceptance score:", result.score);  # 0.72
print("Active rules:", result.active_rules);
```

### Audit trail

Combine with `@trace` to produce a reasoning tree behind the verdict.

```dri
use dri_toulmin;

# branch history from @trace-annotated functions is merged with the argument tree
ToulminResult result = engine.evaluate("is_approved", facts);
AuditTree audit = toulmin.build_audit(result, sys.get_branch_trace());

print(audit.to_string());
# output:
# is_approved [ACCEPTED, score=0.72]
# ├── income_check     [WARRANT, passed]  → income=3500000 > 3000000
# ├── credit_check     [WARRANT, passed]  → credit=720 > 700
# ├── has_debt         [REBUTTAL, fired]  → debt_ratio=0.6 > 0.5
# │   └── collateral_exists [DEFEATS has_debt, fired] → has_collateral=true
```

### Key API

| Function | Description |
|----------|-------------|
| `toulmin.load_rules()` | Load rule graph from binary metadata |
| `engine.evaluate(claim, facts)` | Evaluate claim → `ToulminResult` |
| `toulmin.build_audit(result, trace)` | Build reasoning tree → `AuditTree` |
| `engine.set_weight(rule, weight)` | Adjust rule weight |
| `engine.export_graph(path)` | Export rule graph as JSON |

---

## dri-data — Multi-Format I/O and Conversion

Saves and loads DataFrames, rule-engine results, and tensors in various formats.

### Text serialization

```dri
use dri_data;

DataFrame df = pandas.read_csv("input.csv");

# save as CSV
data.write_csv(df, "output.csv");

# save as JSON
data.write_json(df, "output.json");

# save as XML
data.write_xml(df, "output.xml");

# save Toulmin result as JSON
ToulminResult result = engine.evaluate("is_approved", facts);
data.write_json(result, "audit.json");
```

### Image serialization

```dri
use dri_data;
use dri_plot;

DataFrame df = pandas.read_csv("sales.csv");

# export DataFrame table as PNG image
data.export_table_png(df, "table.png");

# export argument graph as PNG image
AuditTree audit = toulmin.build_audit(result, sys.get_branch_trace());
data.export_graph_png(audit, "argument_graph.png");
```

### Key API

| Function | Description |
|----------|-------------|
| `data.write_csv(obj, path)` | Save as CSV |
| `data.write_json(obj, path)` | Save as JSON |
| `data.write_xml(obj, path)` | Save as XML |
| `data.read_json(path)` | Load JSON with type inference |
| `data.export_table_png(df, path)` | DataFrame table → PNG |
| `data.export_graph_png(tree, path)` | Argument tree → PNG |

---

## dri-plot — High-Performance Data Visualization

### Chart engine

```dri
use dri_plot;

DataFrame df = pandas.read_csv("sales.csv");

# line chart
plot.line(df, x="month", y="revenue", title="Monthly Revenue");

# bar chart
plot.bar(df, x="region", y="profit", title="Profit by Region");

# scatter plot
plot.scatter(df, x="cost", y="revenue", color="region");

# heatmap (tensor data)
tensor<10, 10, double> matrix = ...;
plot.heatmap(matrix, title="Correlation Matrix");
```

### Dashboard viewer

`plot.show()` displays charts in a GUI window or a web-based interface.

```dri
use dri_plot;

DataFrame df = pandas.read_csv("monthly.csv");

# compose multiple charts into a dashboard
Dashboard dash = plot.dashboard("Monthly Report");

dash.add(plot.line(df, x="month", y="revenue"));
dash.add(plot.bar(df, x="region", y="profit"));
dash.add(plot.scatter(df, x="cost", y="revenue"));

# display in a GUI window
dash.show();

# export as HTML
dash.export_html("dashboard.html");

# export as PNG
dash.export_png("dashboard.png");
```

### Key API

| Function | Description |
|----------|-------------|
| `plot.line(df, x, y)` | Line chart |
| `plot.bar(df, x, y)` | Bar chart |
| `plot.scatter(df, x, y)` | Scatter plot |
| `plot.heatmap(tensor)` | Heatmap |
| `plot.dashboard(title)` | Create a dashboard |
| `dash.add(chart)` | Add a chart |
| `dash.show()` | Display in GUI or web viewer |
| `dash.export_html(path)` | Export as HTML |
| `dash.export_png(path)` | Export as PNG |

---

## Full Pipeline Example

```dri
use dri_pandas;
use dri_toulmin;
use dri_data;
use dri_plot;

# 1. load data
DataFrame df = pandas.read_csv("loan_applications.csv");

# 2. preprocess
DataFrame processed = df
    .query("age >= 18")
    .assign("debt_ratio", |row| -> row["debt"] / row["income"]);

# 3. rule-engine evaluation
RuleEngine engine = toulmin.load_rules();
list<ToulminResult> results = [];

for (row of processed.iter_rows()) {
    Map<String, double> facts = row.to_map();
    ToulminResult r = engine.evaluate("is_approved", facts);
    lst.push(results, r);
};

# 4. visualise results
DataFrame scored = processed.assign("score", |i| -> results[i].score);
plot.bar(scored, x="applicant_id", y="score", title="Loan Approval Scores").show();

# 5. save audit trail
AuditTree audit = toulmin.build_audit(results[0], sys.get_branch_trace());
data.export_graph_png(audit, "audit_trail.png");
data.write_json(audit, "audit_trail.json");
```

> For the annotation system (`@warrant`, `@rebuttal`, `@defeats`, `@trace`) see [13_annotations.md](13_annotations.md).

---
