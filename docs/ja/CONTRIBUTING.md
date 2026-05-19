# dri へのコントリビュートガイド

dri は教育用かつ HPC システム言語を目指すオープンソースプロジェクトです。

私たちが大切にしている価値:

- **シンプルさ** — 学びやすい文法
- **性能** — C++ 並みの実行速度
- **安全性** — 所有権・借用モデル、型安全性
- **教育向け** — 明確なエラーメッセージ、直感的な概念
- **並列処理の身近さ** — `parallel`、`simd for`、`async/await` を手軽に

---

## はじめに

### リポジトリのクローン

```bash
git clone https://github.com/drewdrew0414/dri-compiler.git
cd dri-compiler
```

### ビルド

```bash
cmake -B build
cmake --build build
```

### テスト

```bash
cd build
ctest --output-on-failure
```

---

## プロジェクト構成

```
dri-compiler/
├── main.cpp              # コンパイラのエントリポイント
├── compiler/
│   └── src/              # コンパイラ実装 (Lexer, Parser, AST, Codegen, Analyzer, Incremental)
├── docs/
│   ├── en/               # 英語ドキュメント
│   ├── ja/               # 日本語ドキュメント
│   └── ko/               # 韓国語ドキュメント (原本)
├── installer/
│   ├── linux/            # Linux/macOS インストーラとパッケージャ
│   └── window/           # Windows インストーラ (PS1 + WinForms)
├── CMakeLists.txt
└── LICENSE
```

---

## 貢献できる領域

### コンパイラコア

| 領域 | 内容 |
|------|------|
| Lexer | トークナイズ、キーワード追加 |
| Parser | 文法規則、AST 構築 |
| AST | ノード最適化、Visitor パターン |
| 型システム | 型推論、ジェネリクス、単位型 |
| C++ コード生成 | トランスパイル品質の改善 |

### HPC ランタイム

| 領域 | 内容 |
|------|------|
| SIMD | AVX2 / AVX-512 / NEON の自動ベクトル化 |
| 並列 | Work-Stealing スレッドプールの改善 |
| NUMA | メモリ局所性の最適化 |
| tensor | 数値演算の高速化 |

### 言語機能

| 機能 | 状態 |
|------|------|
| `diff.*` 自動微分 | 設計中 |
| `@units_check` 単位型 | 設計中 |
| `drvpm` パッケージマネージャ | 設計中 |
| LLVM バックエンド | 将来計画 |

### ドキュメント

- 韓国語の教育ドキュメント (`docs/ko/education/`) を充実
- `docs/en/` と `docs/ja/` を韓国語版の更新に追従させる
- サンプルコードを追加

---

## コントリビュートの流れ

1. まず Issue を開いて作業内容を共有します。
2. `main` ブランチから分岐します:
   ```bash
   git checkout -b feat/my-feature
   ```
3. 変更をコミットします:
   ```bash
   git commit -m "feat: 機能の説明"
   ```
4. Pull Request を作成します。

### コミットメッセージの形式

```
<type>: <1 行の要約>

[任意] 詳細
```

| Type | 用途 |
|------|------|
| `feat` | 新機能 |
| `fix` | バグ修正 |
| `docs` | ドキュメントのみ |
| `refactor` | リファクタリング |
| `test` | テスト追加・修正 |
| `chore` | ビルド・設定変更 |

---

## コードスタイル

- C++20 をターゲットとします。
- プロジェクトの `clang-format` 設定に従います。
- 関数名・クラス名は `snake_case` を使用します。
- 公開 API には簡潔なコメントを付けます。

---

## コミュニティ (今後予定)

| チャンネル | 内容 |
|------------|------|
| Discord | リアルタイム質問・議論 |
| Playground | ブラウザで dri 実行 |
| Benchmark dashboard | 性能の継続トラッキング |
| 教育サイト | 多言語チュートリアルサイト |

ロードマップ全体は [ROADMAP.md](ROADMAP.md) を参照してください。

---
