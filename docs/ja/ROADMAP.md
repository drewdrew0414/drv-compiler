# dri ロードマップ

dri コンパイラおよび言語エコシステムの開発方針をまとめます。  
*最終更新: 2026-05-19 — v0.1.0 リリース時点*

---

## 現在の状況 (v0.1.0 — トランスパイラ MVP リリース)

C++20 トランスパイラの全パイプライン (Lexer → Parser → 静的解析 → コード生成 →
バックエンド呼び出し) が動作し、dri ソースを実際にビルドして実行ファイルを生成できます。

| 領域 | 状況 |
|------|------|
| 基本文法 (変数、型、入出力) | ✅ 実装済み |
| 制御構文 (`if`, `for`, `while`, `switch`, `match`) | ✅ 実装済み |
| 関数・クラス・継承・トレイト・`impl` | ✅ 実装済み |
| ラムダ / `|>` パイプ / 高階関数 | ✅ 実装済み |
| コレクション (`array`, `list`, `Map`, 基本 `tensor`) | ✅ 実装済み |
| エラー処理 (`try/catch`, `Option`, `Result`, `match`) | ✅ 実装済み |
| メモリモデル (`Own`, `Ref`, `Borrow`, `move`, `@region`) | ✅ 実装済み |
| SIMD ループ (`simd for`) + リダクション | ✅ 実装済み (OpenMP `simd`) |
| 並列ループ (`parallel for`, `parallel simd for`) | ✅ 実装済み (OpenMP) |
| `async / await / spawn` | ✅ 実装済み (`std::async` / スレッド) |
| アノテーション (`@inline`, `@pure`, `@noalias`, `@threadsafe`, `@bench`, `@trace`, `@specialize`, `@fast_math`, `@stack/@heap`, `@warrant/@rebuttal/@defeats`) | ✅ 実装済み |
| `compile_eval` / `static_if` | ✅ 実装済み |
| 組み込み関数 (str, lst, map, math, io, sys, diff.*) | ✅ 実装済み |
| モジュールシステム (`module` / `use`) | ✅ 第一段階のモジュールロード、重複インポート抑制 |
| `#line` 指示子 + ソースマップ (JSON) | ✅ 実装済み |
| 静的解析 (スレッド安全性・エイリアシング・`@packed` 継承・`atomic<String>`) | ✅ 実装済み |
| 増分ビルドキャッシュ (CRC32 + mtime) | ✅ 実装済み |
| クロスコンパイル (`--target`, `--sysroot`, `--cross-cxx`) | ✅ 実装済み |
| バックエンド (`clang++` / `g++`, `-fopenmp`, `-flto`, `-march=native`) | ✅ 実装済み |
| Windows / macOS / Linux インストーラ | ✅ 初版リリース (PS1 + WinForms + Bash) |

> **CLI オプション一覧**: `--exe / --cpp / --check / --debug / --release /
> --opt N / --native / --lto / --trace / --incremental / --cache-dir /
> --no-analyze / --target / --sysroot / --cross-cxx / --source-map / -D<FLAG>`

---

## v0.2 — ツールチェーンの安定化 (次のマイルストーン)

**目標**: リリースされたトランスパイラを実運用に耐える品質に磨き上げる。

### コンパイラ品質
- [ ] エラーメッセージの統一 — 全段階で `file:line:col: kind: msg` を遵守
- [ ] 診断メッセージの多言語化 (`--lang ko|en|ja`)
- [ ] `--source-map` 出力の検証ツール
- [ ] 回帰テストスイート (`tests/cases/*.dri` + 期待出力比較)
- [ ] CI マトリクス (Linux / macOS / Windows × clang / g++)

### ビルドシステム
- [ ] `dri build` マルチファイルプロジェクトマニフェスト (`dri.toml`)
- [ ] システム静的ライブラリのリンク (`--link math,m,pthread`)
- [ ] インストーラの自動署名 (Apple notarization、Windows Authenticode)
- [ ] `installer/linux/build-installer.sh` 成果物の R2 レジストリへの自動アップロード

### 安全性
- [ ] `--strict` モード (警告をエラーに昇格)
- [ ] 整数オーバーフローチェック (`@checked_arith` アノテーション)
- [ ] モジュールインポートサイクル検出

---

## v0.3 — 型システムの強化

**目標**: ジェネリクスと所有権を単なる「通過」マッピングではなく、実際に *検査される* システムにする。

- [ ] 真のジェネリック関数・クラス (現状は C++ テンプレートに渡すのみ)
- [ ] トレイト境界の検証 (`T implements Printable` → 実装が存在することを要求)
- [ ] 所有権フロー解析 (`move` 後の使用はコンパイルエラー)
- [ ] `Borrow<T>` ライフタイム検査 (エスケープ解析)
- [ ] `dim` 物理単位型の衝突検出 (`@units_check`)
- [ ] リフレクション (`reflect.type_of`, `reflect.fields`)
- [ ] `Option<T>` / `Result<T>` の未処理に対する警告

---

## v0.4 — HPC ランタイム拡張

**目標**: 単純な OpenMP を超え、実際の HPC ワークロード向けに最適化されたランタイムを実現。

- [ ] AVX-512 自動ディスパッチ (`__attribute__((target("avx512f")))` クローン関数)
- [ ] Work-Stealing スレッドプール (`parallel for of` コレクション用)
- [ ] `tensor<N, T>` 固定階数 SIMD 最適化配列
- [ ] `simd.fmadd`, `math.invsqrt`, `bits.popcount` のインライン化
- [ ] `mem.prefetch`, `sys.affinity`, `sys.numa` メモリアノテーション
- [ ] ノンブロッキング I/O (`async io.read_file` → `io_uring` / `kqueue` / `IOCP`)
- [ ] `--trace` 出力を Chrome `tracing` フォーマットに対応 (現状はスタブ)

---

## v0.5 — コンパイル時メタプログラミング

**目標**: ユーザー自身がコンパイル時ロジックを表現できる基盤を提供。

- [ ] `diff.forward` / `diff.numerical` / `diff.hessian` 自動微分のコード生成
- [ ] `@bench` 計測結果をビルド成果物 JSON に添付
- [ ] `@specialize<T=float, double>` による型ごとの SIMD 特殊化
- [ ] `static_if` 内で使えるコンパイル時標準ライブラリ
- [ ] `@warrant / @rebuttal / @defeats` のメタデータを ELF / Mach-O セクションに埋め込み
- [ ] `extern "FFI"` で Rust・Zig との安全な連携 (`Own/Ref` 自動マッピング)
- [ ] `@unsafe_legacy extern "C"` のレガシー防衛層 (シグナルハンドラ自動注入)

---

## v0.6 — ライブラリエコシステム

**目標**: dri-pandas、dri-toulmin、dri-data、dri-plot の主要 4 ライブラリを実装。

- [ ] **dri-pandas** — DataFrame パイプライン
  - [ ] Apache Arrow / Rust Polars コアの FFI ラップ
  - [ ] `.query()`, `.assign()`, `.groupby()`, `.mean()` のメソッドチェーン
  - [ ] `df.to_tensor()` テンソル変換
- [ ] **dri-toulmin** — 論証ルールエンジン ([参考](https://github.com/park-jun-woo/toulmin))
  - [ ] 信頼度計算: $Acc(a) = \frac{w(a)}{1 + \sum Acc(attackers)}$
  - [ ] `toulmin.load_rules()` バイナリメタデータの読み込み
  - [ ] `toulmin.build_audit()` 監査トレースツリー
- [ ] **dri-data** — マルチフォーマット I/O
  - [ ] CSV、JSON、XML 高速シリアライズ
  - [ ] DataFrame → PNG エクスポート
  - [ ] 論証ツリー → PNG エクスポート
- [ ] **dri-plot** — データ可視化
  - [ ] line / bar / scatter / heatmap チャートエンジン
  - [ ] `plot.dashboard()` ダッシュボード構成
  - [ ] GUI ウィンドウ (`show()`) と HTML/PNG エクスポート

---

## v1.0 — パッケージマネージャとセルフホスティング

**目標**: 実用的な dri エコシステム完成と、dri 自身によるコンパイラ実装。

- [ ] `drvpm` パッケージマネージャ
  - [ ] `drvpm init` — プロジェクト初期化
  - [ ] `drvpm install` — SemVer + ロックファイルでの依存解決
  - [ ] `drvpm publish` — R2 レジストリへの公開
  - [ ] `drvpm build/test/run` — タスクランナー
- [ ] 標準ライブラリパッケージ (`math`, `tensor`, `http`, `json`)
- [ ] LSP サーバ — IDE 補完・診断・hover (`dri-lsp`)
- [ ] Playground — ブラウザで dri 実行 (WASM コンパイラ + WASI ランタイム)
- [ ] セルフホスティング (dri で書かれた dri コンパイラの初版ブートストラップ)

---

## 長期計画 (v1.1+)

| 計画 | 説明 |
|------|------|
| LLVM バックエンド | C++ トランスパイルを介さず LLVM IR を直接生成 |
| モジュール単位の増分コンパイル | `*.dri.bc` キャッシュで変更モジュールのみ再ビルド |
| WASM ターゲット | WebAssembly 出力 (Playground と共有) |
| GPU カーネル | CUDA / SPIR-V / Metal バックエンド (`@gpu parallel for`) |
| 対話型 REPL | LSP ベースの即時実行 |
| インタープリタモード | スクリプト用途 (`dri --run script.dri`) |
| デバッガ統合 | DWARF 行マッピング + gdb/lldb カスタムプリンタ |

---

## コントリビュート

特定の項目に貢献したい場合はまず Issue を開いてください。  
方法は [CONTRIBUTING.md](CONTRIBUTING.md) を参照。

---
