# 外部ライブラリ
> dri エコシステムの主要標準ライブラリ 4 種。`drvpm install` でインストールし、それぞれ独立したモジュールとして配布される。

---

## ライブラリ一覧

| パッケージ | 役割 |
|-----------|------|
| `dri-pandas` | DataFrame およびパイプライン演算 |
| `dri-toulmin` | トゥールミン論証ベースのルールエンジン |
| `dri-data` | マルチフォーマット I/O および変換 |
| `dri-plot` | 高性能データ可視化 |

```bash
drvpm install dri-pandas
drvpm install dri-toulmin
drvpm install dri-data
drvpm install dri-plot
```

---

## 互換性マトリクス

### dri コンパイラバージョン

| パッケージ | 最低 dri | 推奨 dri | 備考 |
|-----------|---------|---------|------|
| `dri-pandas` | `≥ 0.6.0` | `≥ 1.0.0` | Apache Arrow FFI が必要 |
| `dri-toulmin` | `≥ 0.5.0` | `≥ 0.6.0` | `@warrant`/`@trace` アノテーションが必要 |
| `dri-data` | `≥ 0.6.0` | `≥ 1.0.0` | `dri-pandas` に依存 |
| `dri-plot` | `≥ 0.6.0` | `≥ 1.0.0` | `dri-data` に依存 |

### C++ バックエンド

| パッケージ | clang | GCC | MSVC | 備考 |
|-----------|-------|-----|------|------|
| `dri-pandas` | `≥ 14` | `≥ 12` | `≥ 19.30` | AVX2 オプション対応 |
| `dri-toulmin` | `≥ 12` | `≥ 11` | `≥ 19.28` | — |
| `dri-data` | `≥ 14` | `≥ 12` | `≥ 19.30` | libpng, libjpeg 必要 |
| `dri-plot` | `≥ 14` | `≥ 12` | `≥ 19.30` | OpenGL / WebGPU オプション対応 |

### プラットフォーム

| パッケージ | Linux | macOS | Windows | 備考 |
|-----------|-------|-------|---------|------|
| `dri-pandas` | ✅ | ✅ | ✅ | WSL2 推奨 |
| `dri-toulmin` | ✅ | ✅ | ✅ | — |
| `dri-data` | ✅ | ✅ | ✅ | PNG エクスポートは libpng 必要 |
| `dri-plot` | ✅ | ✅ | ✅ | GUI: X11 / AppKit / Win32 |

### パッケージ間の依存関係

```
dri-plot
  └── dri-data
        └── dri-pandas
              └── [Apache Arrow / Rust Polars — FFI]
dri-toulmin
  └── [独立 — 他パッケージへの依存なし]
```

`drvpm.drii` で `dri-plot` のみ追加しても `dri-data`、`dri-pandas` が自動インストールされます。

---

## dri-pandas — DataFrame およびパイプライン演算

### バックエンド

Apache Arrow または Rust Polars コアを `extern "FFI"` でラップします。  
列ベースのメモリレイアウト (`@layout_soa`) とゼロコピースライシングで大容量データを処理します。

```dri
use dri_pandas;

# CSV ロード
DataFrame df = pandas.read_csv("sales.csv");

# クエリと変換
DataFrame result = df
    .query("revenue > 1000")
    .assign("profit", |row| -> row["revenue"] - row["cost"])
    .groupby("region")
    .mean("profit");

print(result);
```

### 主要メソッド

| メソッド | 説明 |
|---------|------|
| `pandas.read_csv(path)` | CSV ファイル → DataFrame |
| `pandas.read_json(path)` | JSON ファイル → DataFrame |
| `df.query(expr)` | 条件でフィルタリング |
| `df.assign(col, lambda)` | 派生列を追加 |
| `df.groupby(col)` | グループ集計を開始 |
| `df.mean(col)` | 列の平均 |
| `df.sum(col)` | 列の合計 |
| `df.get_column(name)` | 単一列を抽出 → Series |
| `df.head(n)` | 先頭 n 行 |
| `df.shape()` | (行数, 列数) |
| `df.to_tensor()` | 数値列 → `tensor` に変換 |

### メソッドチェーン

```dri
use dri_pandas;

DataFrame df = pandas.read_csv("students.csv");

DataFrame top = df
    .query("score >= 90")
    .assign("grade", |row| -> "A")
    .groupby("class")
    .mean("score");

for (row of top.iter_rows()) {
    print(row["class"], "平均:", row["score"]);
};
```

---

## dri-toulmin — 論証ベースのルールエンジン

> 参考: [park-jun-woo/toulmin](https://github.com/park-jun-woo/toulmin)

### 概要

トゥールミン (Toulmin) の論証モデルに基づいたルール評価エンジンです。  
`@warrant`、`@rebuttal`、`@defeats` アノテーションで定義されたルールグラフを実行時に読み込み、  
信頼度スコア (Acceptance Score) を計算して最終判断を下します。

### 信頼度計算アルゴリズム

$$Acc(a) = \frac{w(a)}{1 + \sum_{b \in attackers(a)} Acc(b)}$$

- `w(a)` — ルール a の基本重み
- `attackers(a)` — a を攻撃 (rebuttal/defeats) するルールの集合
- 反復計算で収束 (デフォルト 10 回)

### 使い方

```dri
use dri_toulmin;

# @warrant / @rebuttal / @defeats アノテーションでルールを定義
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

# ルールエンジンの初期化 — バイナリメタデータからルールグラフを自動ロード
RuleEngine engine = toulmin.load_rules();

# 入力データ
Map<String, double> facts = {};
map.set(facts, "income", 3500000.0);
map.set(facts, "credit_score", 720.0);
map.set(facts, "debt_ratio", 0.6);
map.set(facts, "has_collateral", 1.0);

# 評価
ToulminResult result = engine.evaluate("is_approved", facts);

print("承認:", result.accepted);             # true
print("信頼度スコア:", result.score);         # 0.72
print("活性ルール:", result.active_rules);
```

### 監査トレース (Audit Trail)

`@trace` アノテーションと組み合わせると、最終判断の根拠ツリーを生成します。

```dri
use dri_toulmin;

# @trace が付いた関数の分岐履歴を論証ツリーと結合
ToulminResult result = engine.evaluate("is_approved", facts);
AuditTree audit = toulmin.build_audit(result, sys.get_branch_trace());

print(audit.to_string());
# 出力:
# is_approved [ACCEPTED, score=0.72]
# ├── income_check     [WARRANT, passed]  → income=3500000 > 3000000
# ├── credit_check     [WARRANT, passed]  → credit=720 > 700
# ├── has_debt         [REBUTTAL, fired]  → debt_ratio=0.6 > 0.5
# │   └── collateral_exists [DEFEATS has_debt, fired] → has_collateral=true
```

### 主要 API

| 関数 | 説明 |
|------|------|
| `toulmin.load_rules()` | バイナリメタデータからルールグラフをロード |
| `engine.evaluate(claim, facts)` | 主張を評価 → `ToulminResult` |
| `toulmin.build_audit(result, trace)` | 根拠ツリーを生成 → `AuditTree` |
| `engine.set_weight(rule, weight)` | ルールの重みを調整 |
| `engine.export_graph(path)` | ルールグラフを JSON で保存 |

---

## dri-data — マルチフォーマット I/O および変換

DataFrame、ルールエンジンの結果、テンソルをさまざまなフォーマットで保存・読み込みます。

### テキストシリアライズ

```dri
use dri_data;

DataFrame df = pandas.read_csv("input.csv");

# CSV として保存
data.write_csv(df, "output.csv");

# JSON として保存
data.write_json(df, "output.json");

# XML として保存
data.write_xml(df, "output.xml");

# トゥールミン結果を JSON として保存
ToulminResult result = engine.evaluate("is_approved", facts);
data.write_json(result, "audit.json");
```

### 画像シリアライズ

```dri
use dri_data;
use dri_plot;

DataFrame df = pandas.read_csv("sales.csv");

# DataFrame テーブルを PNG 画像としてエクスポート
data.export_table_png(df, "table.png");

# 論証グラフを PNG 画像としてエクスポート
AuditTree audit = toulmin.build_audit(result, sys.get_branch_trace());
data.export_graph_png(audit, "argument_graph.png");
```

### 主要 API

| 関数 | 説明 |
|------|------|
| `data.write_csv(obj, path)` | CSV として保存 |
| `data.write_json(obj, path)` | JSON として保存 |
| `data.write_xml(obj, path)` | XML として保存 |
| `data.read_json(path)` | JSON を型推論でロード |
| `data.export_table_png(df, path)` | DataFrame テーブル → PNG |
| `data.export_graph_png(tree, path)` | 論証ツリー → PNG |

---

## dri-plot — 高性能データ可視化

### チャートエンジン

```dri
use dri_plot;

DataFrame df = pandas.read_csv("sales.csv");

# 折れ線グラフ
plot.line(df, x="month", y="revenue", title="月別売上");

# 棒グラフ
plot.bar(df, x="region", y="profit", title="地域別利益");

# 散布図
plot.scatter(df, x="cost", y="revenue", color="region");

# ヒートマップ (テンソルデータ)
tensor<10, 10, double> matrix = ...;
plot.heatmap(matrix, title="相関行列");
```

### ダッシュボードビューワー

`plot.show()` は GUI ウィンドウまたはウェブベースのインターフェースでチャートを表示します。

```dri
use dri_plot;

DataFrame df = pandas.read_csv("monthly.csv");

# 複数のチャートをダッシュボードに構成
Dashboard dash = plot.dashboard("月次レポート");

dash.add(plot.line(df, x="month", y="revenue"));
dash.add(plot.bar(df, x="region", y="profit"));
dash.add(plot.scatter(df, x="cost", y="revenue"));

# GUI ウィンドウで表示
dash.show();

# HTML ファイルとしてエクスポート
dash.export_html("dashboard.html");

# PNG 画像としてエクスポート
dash.export_png("dashboard.png");
```

### 主要 API

| 関数 | 説明 |
|------|------|
| `plot.line(df, x, y)` | 折れ線グラフ |
| `plot.bar(df, x, y)` | 棒グラフ |
| `plot.scatter(df, x, y)` | 散布図 |
| `plot.heatmap(tensor)` | ヒートマップ |
| `plot.dashboard(title)` | ダッシュボードの作成 |
| `dash.add(chart)` | チャートを追加 |
| `dash.show()` | GUI またはウェブビューワーで表示 |
| `dash.export_html(path)` | HTML としてエクスポート |
| `dash.export_png(path)` | PNG としてエクスポート |

---

## 全パイプラインの例

```dri
use dri_pandas;
use dri_toulmin;
use dri_data;
use dri_plot;

# 1. データロード
DataFrame df = pandas.read_csv("loan_applications.csv");

# 2. 前処理
DataFrame processed = df
    .query("age >= 18")
    .assign("debt_ratio", |row| -> row["debt"] / row["income"]);

# 3. ルールエンジンの評価
RuleEngine engine = toulmin.load_rules();
list<ToulminResult> results = [];

for (row of processed.iter_rows()) {
    Map<String, double> facts = row.to_map();
    ToulminResult r = engine.evaluate("is_approved", facts);
    lst.push(results, r);
};

# 4. 結果の可視化
DataFrame scored = processed.assign("score", |i| -> results[i].score);
plot.bar(scored, x="applicant_id", y="score", title="ローン承認スコア").show();

# 5. 監査トレースの保存
AuditTree audit = toulmin.build_audit(results[0], sys.get_branch_trace());
data.export_graph_png(audit, "audit_trail.png");
data.write_json(audit, "audit_trail.json");
```

> アノテーションシステム (`@warrant`, `@rebuttal`, `@defeats`, `@trace`) については [13_annotations.md](13_annotations.md) を参照してください。

---
