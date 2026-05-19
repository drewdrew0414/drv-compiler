# dri言語の紹介
### Learn like Python. Scale like C++.

---

## 言語の特徴

driは次の目標に重点を置いた言語です。

| 目標 | 比較言語 | 説明 |
|------|---------|------|
| 高い生産性 | Python | 直感的で簡潔な文法 |
| 強い安全性 | Rust | 所有権・借用モデル + 型安全 |
| 最高の性能 | C++ | ゼロコスト抽象化、C++17トランスパイル |
| 数値計算 | Julia | 組み込みSIMD、tensor演算、自動微分 |

---

## クイックスタート — Hello World

```dri
# hello.dri
print("Hello, dri!");

int x = 10;
double pi = 3.14159;
print("x =", x, "pi =", pi);

for (i in 0..5) {
    print(i);
};
```

コンパイルおよび実行:

```bash
dri hello.dri --exe hello
./hello
```

出力:
```
Hello, dri!
x = 10 pi = 3.14159
0
1
2
3
4
```

---

## 動作の仕組み

driのソースコードはC++17コードに**トランスパイル**された後、C++コンパイラでビルドされます。

```
hello.dri  →  [driコンパイラ]  →  hello.cpp  →  [clang/g++]  →  hello
```

- SIMDおよび並列機能は、clang/g++の最適化パイプラインをそのまま活用します。
- `#line`ディレクティブによって、エラー位置は元の`.dri`ファイルを指します。

---

## CLIオプション

```
dri <入力.dri> [オプション]
```

| オプション | 説明 |
|------|------|
| `--exe <出力ファイル>` | 実行ファイルを生成 |
| `--cpp <出力.cpp>` | C++トランスパイル結果のみ保存 |
| `--check` | コンパイルせず文法・型検査のみ実行 |
| `--debug` | デバッグビルド(`-O0`、範囲検査を有効化) |
| `--release` | リリースビルド(`-O3`、`-march=native`、LTO) |
| `--opt <0~3>` | 最適化レベルを手動指定 |
| `--native` | `-march=native`を有効化 |
| `--lto` | Link-Time Optimizationを有効化 |
| `--trace <ファイル>` | parallelループのトレースファイルを生成(Chrome DevTools形式) |
| `-D<フラグ>` | コンパイル時フラグを定義(static_if条件で使用) |
| `--target <triple>` | クロスコンパイル先(`x86_64-linux-gnu`、`aarch64-linux-gnu`など) |
| `--sysroot <パス>` | ターゲットOSのシステムヘッダ/ライブラリのパス |
| `--cross-cxx <コンパイラ>` | クロスコンパイラのバイナリを明示 |
| `--source-map <ファイル>` | JSONソースマップを生成(C++行 → dri行) |
| `--incremental` | 変更されたファイルのみ再コンパイル |
| `--no-analyze` | 静的解析を無効化 |

---

## PowerShell / CMDのコマンドの違い

driコマンド自体はどのシェルでも同一です。ただし、**ビルド後の実行**(`&&`)構文はシェルごとに異なります。

### コンパイルのみ(全シェル共通)

```
dri main.dri --exe main.exe
dri main.dri --check
dri main.dri --cpp out.cpp
```

### コンパイル後に即実行

| シェル | コマンド |
|----|--------|
| **PowerShell 5.1**(既定) | `& "dri.exe" main.dri --exe main.exe; if ($LASTEXITCODE -eq 0) { & ".\main.exe" }` |
| **PowerShell 7+**(pwsh) | `& "dri.exe" main.dri --exe main.exe && & ".\main.exe"` |
| **CMD** | `dri.exe main.dri --exe main.exe && main.exe` |
| **Bash / WSL** | `dri main.dri --exe main && ./main` |

> PowerShell 5.1には`&&`演算子がありません(PowerShell 7で追加されました)。
> VSCode拡張の**dri: Run File**ボタンは、アクティブなターミナルの種類を自動検出し、正しい構文を使用します。

### 環境変数の確認

| 項目 | PowerShell | CMD |
|------|------------|-----|
| PATHの確認 | `$env:PATH -split ';'` | `echo %PATH%` |
| インストールパスの確認 | `where.exe dri` | `where dri` |
| driバージョンの確認 | `dri --version` | `dri --version` |

```bash
# 基本的な使用
dri main.dri --exe main --release
dri main.dri --cpp out.cpp --check
dri main.dri --exe main --debug
dri main.dri --exe main --opt 2 --native --lto
dri main.dri --exe main --release --trace trace.json
```

---

## 言語ルールの要約

| ルール | 内容 |
|------|------|
| 文の終端 | すべての文は`;`で終わります |
| ブロック | `{ }`の波括弧が必須 |
| コメント | `#`は単一行、`!# ... ##`は複数行 |
| ファイル拡張子 | `.dri` |
| エントリポイント | ファイルの最上位に記述(別途`main`関数は不要) |

```dri
!# 複数行コメントは
   このように書きます。
##

# 単一行コメント
int x = 42;   # インラインコメント
print(x);
```

> 変数・型・入出力の基本文法は[[02_basics.md](02_basics.md)]を参照してください。

---

## クロスコンパイル(Cross-Compilation)

driコンパイラは`--target`フラグを使って、現在のOSとは異なるプラットフォーム向けのバイナリをビルドできます。

### サポートされているターゲット

| ターゲットトリプル | 対象環境 | 説明 |
|------------|---------|------|
| `x86_64-linux-gnu` | Linux x86_64 | HPCサーバー、AWS/GCP EC2 |
| `aarch64-linux-gnu` | ARM64 Linux | AWS Graviton、Raspberry Pi 5 |
| `x86_64-w64-mingw32` | Windows 64-bit | Windowsクロスビルド(Linux→Win) |
| `riscv64-linux-gnu` | RISC-V Linux | 組み込みLinuxボード |
| `wasm32-unknown-emscripten` | WebAssembly | ブラウザで実行 |

### 使用例

```bash
# Windows開発PC上でLinux x86_64 AVX-512バイナリをビルド
dri main.dri --exe main_linux --target x86_64-linux-gnu --release

# ARM64サーバー向けビルド(AWS Graviton)
dri main.dri --exe main_arm64 --target aarch64-linux-gnu --release

# sysrootを指定(クロスヘッダ/ライブラリのパス)
dri main.dri --exe out --target aarch64-linux-gnu --sysroot /opt/aarch64-sysroot

# クロスコンパイラを明示指定
dri main.dri --exe out --cross-cxx aarch64-linux-gnu-g++
```

### コンパイラの自動検出順序

1. `clang++`がある場合は`--target=<triple>`フラグで処理(最も柔軟)
2. なければ`<triple>-g++`バイナリを検索(例: `aarch64-linux-gnu-g++`)
3. どちらも無い場合はエラー + インストール方法の案内

```bash
# UbuntuでのARM64クロスコンパイラのインストール
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

---

## エラー逆追跡(Source Mapping)

driコンパイラは、生成されたC++コードに`#line`ディレクティブを自動挿入します。C++コンパイラやデバッガがエラーを報告する際、`.dri`ファイルの正確な行番号が表示されます。

```bash
# 既定: #lineディレクティブを自動挿入(常に有効)
dri main.dri --exe main

# エラー発生時の出力例
main.dri:12:5: error: 'undefined_var' was not declared in this scope
```

### JSONソースマップの生成

```bash
dri main.dri --cpp out.cpp --source-map out.map.json
```

`out.map.json`の形式:
```json
{
  "version": 3,
  "sourceFile": "main.dri",
  "mappings": [
    {"cppLine": 45, "driFile": "main.dri", "driLine": 12},
    {"cppLine": 52, "driFile": "main.dri", "driLine": 15}
  ]
}
```

### GDBデバッグ

`#line`ディレクティブのおかげで、GDBも`.dri`ファイルの位置を追跡します:

```bash
dri main.dri --exe main --debug --source-map main.map.json
gdb ./main
(gdb) run
# セグフォルト発生時: main.dri:23 と表示
```
