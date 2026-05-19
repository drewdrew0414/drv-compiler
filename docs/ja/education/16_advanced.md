# 高度な機能
> dri の高度な言語機能: モジュールシステム、コンパイル時分岐、自動微分、ビルドプロファイル、パッケージマネージャ。

---

## モジュールシステム — `module` / `use`

dri はファイル単位のモジュールをサポートします。各 `.dri` ファイルは独立したコンパイル単位であり、`public` として宣言されたシンボルのみが外部に公開されます。

### モジュール宣言 — `module`

```dri
# math_utils.dri
module math_utils;

public double fancy_sqrt(double x) {
    return sqrt(x * 2.0);
};

public double lerp(double a, double b, double t) {
    return a + (b - a) * t;
};

private double internal_helper(double x) {
    return x * x;   # 外部からアクセス不可
};
```

### モジュールの使用 — `use`

```dri
# main.dri
use math_utils;                    # モジュール全体をインポート
use math_utils::fancy_sqrt;        # 特定シンボルのみインポート

double result = fancy_sqrt(9.0);
print(result);   # 6.0

double v = math_utils::lerp(0.0, 100.0, 0.5);
print(v);        # 50.0
```

### コンパイラの動作

```
math_utils.dri  →  [独立コンパイル]  →  math_utils.o
main.dri        →  [独立コンパイル]  →  main.o
                →  [リンク]          →  実行ファイル
```

- 変更されたモジュールのみ再コンパイル (増分ビルド)
- シンボルスコープの隔離 — 名前衝突なし
- 循環依存をコンパイル時に検出

---

## static_if — コンパイル時条件分岐

`static_if` は **プリプロセッサマクロではありません**。C++ の `if constexpr` と同様に、AST レベルで評価されます。  
条件が偽のブランチは **パースはされますが、コード生成はスキップされます** — テキストのインライン展開ではありません。

### ビルドフラグ条件 (→ C++ `if constexpr` + ビルドフラグ)

```dri
static_if (RELEASE_BUILD) {
    print("release ビルドです");
} else {
    print("debug ビルドです");
};
```

コンパイル時に `-DRELEASE_BUILD` フラグがあれば then ブランチのみコード生成されます。

> **モジュールシステムとの関係**: `static_if` はテキスト前処理ではないため、  
> `drvpm` で配布される **事前コンパイル済みバイナリモジュール** にも適用できます。  
> ライブラリが `static_if` を使用してもソースコードを完全公開する必要はありません。  
> フラグの値はリンク時に確定します。

### 式条件 (→ C++ `if constexpr`)

```dri
static_if (1 == 1) {
    print("常に真");
};
```

### `static_if` vs 実行時 `if` の比較

| | `static_if` | `if` |
|--|-------------|------|
| 評価タイミング | コンパイル時 | 実行時 |
| 偽ブランチ | コード生成されない | コード生成される |
| 条件の種類 | 定数・ビルドフラグ | あらゆる式 |
| 用途 | プラットフォーム/ビルド分岐 | 通常の条件分岐 |

---

## @bench — 実行時間の計測

関数に `@bench` を付けると、その関数の実行時間を自動計測して stderr に出力します。

```dri
@bench
void heavy_computation() {
    double sum = 0.0;
    for (int i = 0; i < 1000000; i++) {
        sum = sum + math.sqrt(i as double);
    };
};

heavy_computation();
# stderr 出力: [bench] heavy_computation: 12.345 ms
```

- RAII 方式で実装: 関数入口でタイマー開始、戻り時に自動計測
- `return` が複数あっても正確に計測可能

---

## @specialize — 型別 SIMD 特殊化

特定の型に対して最適化されたコードパスを生成します。`@specialize` を付けた関数は `DRV_HOT` マーキングされ、自動ベクトル化の優先度が上がります。

```dri
@specialize
double dot_product(tensor a, tensor b) {
    double result = 0.0;
    for (int i = 0; i < length(a); i++) {
        result = result + a[i] * b[i];
    };
    return result;
};
```

---

## 自動微分 (Auto-diff)

dri は言語レベルで自動微分をサポートします。

### diff.forward — 前進モード自動微分

```dri
# f(x) = x^2 + 2x + 1 → f'(x) = 2x + 2
double f(double x) {
    return x * x + 2.0 * x + 1.0;
};

# x = 3.0 における f'(3) = 8.0
var df = diff.forward(f, 3.0);
print(df);  # 8.0
```

### diff.numerical — 数値微分 (中心差分)

```dri
double g(double x) {
    return math.sin(x);
};

var dg = diff.numerical(g, math.pi() / 4.0);
print(dg);  # cos(π/4) ≈ 0.7071...
```

### diff.hessian — 二階導関数

```dri
double h(double x) {
    return x * x * x;  # h''(x) = 6x
};

var d2h = diff.hessian(h, 2.0);
print(d2h);  # 12.0
```

---

## ビルドプロファイル

### Debug モード

```bash
dri main.dri --exe main.exe --debug
```

- 最適化レベル: `-O0`
- Fast-math 無効
- LTO 無効
- 実行時チェック有効 (配列範囲チェックなど)

### Release モード

```bash
dri main.dri --exe main.exe --release
```

- 最適化レベル: `-O3`
- `-march=native` (現在の CPU に合わせた最適化)
- LTO (Link-Time Optimization) 有効
- Fast-math 有効

### 手動設定

```bash
dri main.dri --exe main.exe --opt 2 --native --lto
```

---

## Trace Export — 並列ループの可視化

`parallel` ループのスレッド占有率を Chrome DevTools フォーマットで記録します。

```bash
dri main.dri --exe main.exe --trace trace.json
```

```dri
parallel for (x of data) {
    process(x);
};
```

生成された `trace.json` を `chrome://tracing` または Perfetto で開くと、スレッドごとの実行タイムラインを確認できます。

---

## パッケージマネージャ (drvpm)

独立した実行ファイル `drvpm` として提供されます。Gradle スタイルの依存管理、増分ビルド、R2 レジストリ連携をサポートします。

### 新規プロジェクトの初期化

```bash
drvpm init my-project
cd my-project
```

生成される構成:
```
my-project/
  src/main.dri       # エントリポイント
  tests/main.dri     # テストスイート
  drvpm.drii         # プロジェクトメタデータ
  build/             # ビルド成果物 (自動生成)
```

### drvpm.drii の形式 (Gradle スタイルの依存管理)

`drvpm.drii` は Gradle の `build.gradle` と同等の役割を持ちます。  
依存解決、プラットフォーム条件付き依存、開発専用依存をすべてサポートします。

```ini
[project]
name        = "my-project"
version     = "0.1.0"
author      = "drewdrew0414"
description = "My dri project"
license     = "MIT"
dri_min     = "0.6.0"           # 最低 dri コンパイラバージョン
registry    = "https://drvpm-registry.cloud"

# ── ランタイム依存 (配布に含む) ──────────────────────────────
[dependencies]
math          = "^1.0.0"
tensor        = "~2.3.0"
dri-pandas    = "^0.2.0"
dri-toulmin   = "^0.1.0"
dri-data      = "^0.1.0"
dri-plot      = "^0.1.0"

# ── 開発専用依存 (配布に含まない、Gradle devDependencies 相当) ─
[dev_dependencies]
dri-test      = "^0.1.0"        # テストフレームワーク
dri-bench     = "^0.1.0"        # ベンチマークフレームワーク
dri-lint      = "^0.1.0"        # 静的解析器

# ── プラットフォーム条件付き依存 (Gradle platform() / configurations 相当) ─
[dependencies.platform.linux]
cuda-dri      = "^12.0.0"       # Linux のみ CUDA 連携

[dependencies.platform.windows]
directml-dri  = "^1.0.0"        # Windows のみ DirectML 連携

[dependencies.platform.macos]
metal-dri     = "^3.0.0"        # macOS のみ Metal 連携

# ── フィーチャーフラグ条件付き依存 ────────────────────────────
[dependencies.feature.gpu]
cuda-dri      = "^12.0.0"
opencl-dri    = "^3.0.0"

[dependencies.feature.hpc]
mpi-dri       = "^4.0.0"
blas-dri      = "^3.10.0"

# ── リポジトリ設定 (Gradle repositories 相当) ────────────────
[repositories]
central   = "https://drvpm-registry.cloud"
local     = "./local_packages"
mirror    = "https://mirror.company.internal/drvpm"

# ── 依存解決戦略 ─────────────────────────────────────────────
[resolution]
strategy    = "newest-compatible"   # newest-compatible | strict | lowest
fail_on_conflict = true

# ── タスク定義 (Gradle tasks 相当) ───────────────────────────
[tasks]
build   = "dri src/main.dri --exe build/my-project --release"
test    = "dri tests/main.dri --exe build/test && build/test"
lint    = "dri src/main.dri --check"
bench   = "dri bench/main.dri --exe build/bench --release && build/bench"
docs    = "dridoc src/ --out docs/api/"
deploy  = "drvpm publish"
```

### 依存継承 — `extend` (Gradle BOM 相当)

共通設定を親プロジェクトから継承します。

```ini
[project]
name   = "my-service"
extend = "company-base-drii@^1.0.0"   # 親設定を継承

[dependencies]
# 親の dri-pandas、dri-data 等のバージョンを自動継承
# オーバーライドが必要なものだけ明示
dri-pandas = "^0.3.0"   # 親バージョンをオーバーライド
```

### バージョンカタログ — `[versions]`

チーム全体のバージョンを一箇所で管理します (Gradle Version Catalog 相当)。

```ini
[versions]
dri-pandas-ver  = "0.2.3"
dri-plot-ver    = "0.1.5"
arrow-core-ver  = "14.0.0"

[dependencies]
dri-pandas = { version = "$dri-pandas-ver", features = ["arrow", "parquet"] }
dri-plot   = { version = "$dri-plot-ver",   features = ["webgl"] }
```

### 主要コマンド

```bash
drvpm install              # 依存インストール (drvpm.lock 生成)
drvpm install math@^1.0.0  # 特定パッケージを追加

drvpm build                # 増分ビルド
drvpm build --debug
drvpm build --release

drvpm run                  # 実行
drvpm test                 # テスト

drvpm search math          # パッケージ検索
drvpm list                 # インストール済みパッケージ一覧
drvpm clean                # ビルド成果物のクリーンアップ
```

### ロックファイル (決定論的ビルド)

`drvpm install` 時に `drvpm.lock` を自動生成:

```ini
[meta]
generated = "2026-05-16T10:00:00Z"

[math]
version = "1.2.3"
hash    = "sha256:abc123..."
url     = "https://drvpm-registry.cloud/packages/math/1.2.3.tar.gz"
```

チームメンバーが `drvpm install` を実行すると、ロックファイルに基づいて正確に同じバージョンがインストールされます。

### SemVer 制約の表記

| 表記 | 意味 |
|------|------|
| `^1.2.0` | `>=1.2.0 <2.0.0` (メジャー固定) |
| `~1.2.0` | `>=1.2.0 <1.3.0` (マイナー固定) |
| `>=1.0.0` | 1.0.0 以上 |
| `1.2.3` | 厳密なバージョン |
| `*` | 任意のバージョン |

### パッケージの公開 (R2)

```bash
drvpm login ACCESS_KEY_ID SECRET_ACCESS_KEY \
  https://ACCOUNT.r2.cloudflarestorage.com my-bucket

drvpm publish
```

### カスタムタスク

```bash
drvpm task lint
drvpm task deploy
```

---

## ソースマップ

コンパイラが生成した C++ コードのエラーを元の dri ソース位置に変換します。`#line` 指示子を通じて、C++ コンパイラが直接 dri ファイルの位置を報告します。

---

## @units_check — 物理単位チェック

`dim` 宣言と組み合わせて、物理単位の整合性をコンパイル時に検査します。

```dri
dim Meter = "m";
dim Second = "s";
dim Kg = "kg";

Meter distance = Meter(10.0);
Second time = Second(5.0);

# 同じ単位同士のみ加算可能
Meter total = distance + Meter(3.0);  # OK
# Meter wrong = distance + time;       # コンパイルエラー！

# 割り算: 異なる単位の結果
double speed = distance / time;  # m/s (double に自動変換)
```

単位タグが異なる値に対しては、C++ の型システムがコンパイル時に加算・減算を拒否します。

> `dim` 物理単位型宣言については [02_basics.md](02_basics.md) を参照してください。

---

## extern "C" — C ライブラリ連携

HPC 分野の既存資産 (BLAS、LAPACK、CUDA など) を dri から直接呼び出せます。  
`extern "C"` ブロック内で C 関数を宣言すると、dri コンパイラは名前マングリングなしに  
C ABI でリンクします。配列引数は生ポインタ (`&arr[0]`) に自動変換されます。

```
extern "C" {
    戻り型 関数名(パラメータ);
};
```

```dri
# BLAS dgemm の宣言 (行列積)
extern "C" {
    void cblas_dgemm(int Order, int TransA, int TransB,
                     int M, int N, int K,
                     double alpha, double[] A, int lda,
                                   double[] B, int ldb,
                     double beta,  double[] C, int ldc);
};

double[] A = new double[1024];
double[] B = new double[1024];
mut double[] C = new double[1024];

# 呼び出し時に配列は &A[0] 形式の生ポインタに自動変換される
cblas_dgemm(101, 111, 111, 32, 32, 32,
            1.0, A, 32, B, 32,
            0.0, C, 32);
```

複数関数の宣言:

```dri
extern "C" {
    void cblas_daxpy(int n, double alpha, double[] x, int incx,
                     double[] y, int incy);
    double cblas_ddot(int n, double[] x, int incx, double[] y, int incy);
    void cblas_dscal(int n, double alpha, double[] x, int incx);
};
```

| dri 型 | C ポインタへの変換 |
|--------|-----------------|
| `double[]` | `double*` (→ `&arr[0]`) |
| `int[]` | `int*` |
| `mut double[]` | `double*` (書き込み可) |
| `Borrow<double[]>` | `const double*` |

> **`mut` キーワード**: C 関数が配列を変更する場合は `mut double[]` で宣言します。  
> 宣言しないとコンパイラは `const` ポインタとして渡すため、C 側の書き込みが UB になります。

---

## extern "FFI" — モダン言語との安全な連携

Rust や Zig など **C 互換 ABI + メタデータを提供する言語** の関数と構造体を dri の型システムに安全にマッピングします。  
コンパイラが型情報を解析し、`Own<T>` / `Ref<T>` バインディングを自動生成します。

```
extern "FFI" {
    use 言語::パス::型;
    fn 関数名(パラメータ) -> 戻り型;
};
```

```dri
extern "FFI" {
    # Rust ライブラリ連携
    use rust::polars::DataFrame;
    use rust::polars::Series;

    fn polars_read_csv(path: String) -> Own<DataFrame>;
    fn polars_filter(df: Ref<DataFrame>, expr: String) -> Own<DataFrame>;
    fn polars_mean(series: Ref<Series>) -> double;
};

Own<DataFrame> df = polars_read_csv("data.csv");
Own<DataFrame> filtered = polars_filter(df, "age > 30");
print(polars_mean(filtered.get_column("salary")));
```

### Own<T> / Ref<T> 自動マッピング規則

| 言語側 | dri マッピング | 意味 |
|--------|------------|------|
| Rust `Box<T>` / `T` (戻り値) | `Own<T>` | 所有権の移転 |
| Rust `&T` | `Borrow<T>` | 不変参照 |
| Rust `&mut T` | `mut Borrow<T>` | 可変参照 |
| Rust `Arc<T>` | `Ref<T>` | 共有所有権 |

---

## @unsafe_legacy extern "C" — レガシー防衛層

古い C や Fortran のライブラリは SegFault やスタックオーバーフローなど **プロセス全体をダウン** させる可能性があります。  
`@unsafe_legacy` を付けると、コンパイラが各呼び出し箇所に **シグナルハンドラと隔離ラッパー** を自動注入します。  
ライブラリがクラッシュしても dri ランタイムがシグナルをキャッチし、`Result<T>` 形式でエラーを返します。

```dri
@unsafe_legacy extern "C" {
    # Fortran LAPACK ルーチン (SegFault の危険あり)
    void dgesv_(int[] N, int[] NRHS, double[] A, int[] LDA,
                int[] IPIV, double[] B, int[] LDB, int[] INFO);

    # 古い画像処理 C ライブラリ
    int legacy_encode(mut double[] buf, int size, String format);
};
```

コンパイラが自動生成するラッパーのイメージ:

```cpp
// 自動生成 C++ ラッパー (概念表示)
Result<void> __dri_safe_dgesv_(...) {
    struct sigaction sa_segv, sa_fpe;
    // SIGSEGV, SIGFPE ハンドラを登録
    // ...
    try {
        dgesv_(...);
        return Ok();
    } catch (...) {
        return Err("legacy call crashed: dgesv_");
    }
}
```

```dri
# dri コードでは Result<> として安全に受け取る
Result<void> res = dgesv_(n, nrhs, A, lda, ipiv, B, ldb, info);
match res {
    Ok(_)  => { print("LAPACK 成功"); };
    Err(e) => { print("レガシークラッシュ:", e); };
};
```

### extern の種類比較

| 構文 | 対象言語 | 安全性 | 型マッピング |
|------|---------|--------|------------|
| `extern "C"` | C (信頼できる) | 開発者の責任 | 手動 |
| `extern "FFI"` | Rust, Zig | コンパイラが自動検証 | 自動 (`Own`/`Ref`) |
| `@unsafe_legacy extern "C"` | C, Fortran (レガシー) | シグナル隔離を自動注入 | 手動 + ラッパー |

---

## 静的解析 (Static Analyzer)

コンパイル時に以下の 4 つのパスを自動実行します。

| パス | 検出項目 |
|------|---------|
| スレッド安全性 | `parallel for` 内の同期なし共有変数への書き込み |
| エイリアシング検証 | `@noalias` 関数呼び出し時の同一変数の重複渡し |
| `@packed` 継承 | `@packed` + `extends` の同時使用 (コンパイルエラー) |
| `atomic<String>` | ポインタのみが原子的である制限の警告 |

すべての警告は `--no-analyze` で無効化できます (リリースビルドの速度向上用):

```bash
dri main.dri --exe app --release --no-analyze
```

---

## 新しい CLI フラグ

| フラグ | 説明 |
|--------|------|
| `--incremental` | 変更されたファイルのみ再コンパイル |
| `--cache-dir <path>` | 増分ビルドキャッシュのディレクトリを指定 |
| `--no-analyze` | 静的解析を無効化 |

---

## クロスコンパイルフラグのまとめ

| フラグ | 説明 |
|--------|------|
| `--target <triple>` | クロスコンパイルターゲットの設定 |
| `--sysroot <path>` | ターゲット OS のシステムヘッダ/ライブラリのパス |
| `--cross-cxx <cc>` | クロスコンパイラのバイナリを明示 |
| `--source-map <f>` | JSON ソースマップファイルの生成 |

### 一般的な使用パターン

```bash
# HPC サーバへのデプロイ (Intel AVX-512 最適化)
dri compute.dri --exe compute_server \
    --target x86_64-linux-gnu \
    --release

# エッジデバイスへのデプロイ (ARM64)
dri edge.dri --exe edge_binary \
    --target aarch64-linux-gnu \
    --sysroot /opt/arm-sysroot \
    --release

# デバッグビルド + ソースマップ
dri main.dri --exe main_debug \
    --debug --source-map main.map.json
```
