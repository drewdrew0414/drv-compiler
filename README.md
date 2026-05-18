# dri Language

> **Learn like Python. Scale like C++.**

---

## 🌐 Language / 언어 / 言語 / 语言

**[English](#english)** | **[한국어](#한국어)** | **[日本語](#日本語)** | **[中文](#中文)**

---

<br>

## English

[한국어](#한국어) | [日本語](#日本語) | [中文](#中文)

### Table of Contents

- [What is dri?](#what-is-dri)
- [Key Features](#key-features)
- [Quick Start](#quick-start)
- [Compilation](#compilation)
- [CLI Options](#cli-options)
- [Language Overview](#language-overview)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [Roadmap](#roadmap)
- [License](#license)

---

### What is dri?

dri is a modern HPC systems programming language that transpiles to C++17.  
It is designed for scientific computing, education, and high-performance applications.

```dri
# hello.dri
print("Hello, dri!");

int x = 10;
double pi = 3.14159;
print(`{x} * {pi} = {x as double * pi}`);

for (i in 0..5) {
    print(i);
};
```

---

### Key Features

| Feature | Description |
|---------|-------------|
| **Productivity** | Python-level syntax — readable, minimal boilerplate |
| **Safety** | Ownership model (`Own`, `Ref`, `Borrow`) + type safety |
| **Performance** | Zero-cost abstractions, transpiles to C++17 |
| **Parallelism** | `parallel for`, `simd for`, `async/await`, `spawn` |
| **Numerics** | Built-in SIMD, `tensor` ops, auto-diff (`diff.*`) |
| **Education** | Clear error messages, intuitive concepts |

---

### Quick Start

```dri
# fib.dri
int fib(int n) {
    if (n <= 1) { return n; };
    return fib(n - 1) + fib(n - 2);
};

print(fib(10));   # 55

# SIMD vector addition
int N = 1024;
double[] a = new double[N];
double[] b = new double[N];
double[] c = new double[N];

simd for (i in 0..N) {
    c[i] = a[i] + b[i];
};

# Parallel processing
list<String> files = [];
lst.push(files, "data1.csv");
lst.push(files, "data2.csv");

parallel for (f of files) {
    String content = io.read_file(f);
    print("loaded:", f, length(content), "bytes");
};
```

---

### Compilation

dri compiles to C++17 and links with clang or g++.

```
dri source  →  [dri compiler]  →  C++17  →  [clang/g++]  →  binary
```

```bash
dri hello.dri --exe hello
./hello
```

Build modes:

```bash
dri main.dri --exe main --debug    # -O0, bounds checking on
dri main.dri --exe main --release  # -O3, -march=native, LTO
```

---

### CLI Options

```
dri <input.dri> [options]
```

| Option | Description |
|--------|-------------|
| `--exe <file>` | Build executable |
| `--cpp <file>` | Output C++ source only |
| `--check` | Type check only, no codegen |
| `--debug` | Debug build (`-O0`, bounds checks) |
| `--release` | Release build (`-O3`, LTO, native) |
| `--opt <0–3>` | Manual optimization level |
| `--trace <file>` | Export parallel loop trace (Chrome DevTools) |
| `-D<FLAG>` | Define compile-time flag for `static_if` |

---

### Language Overview

```dri
# Types
int x = 42;
double pi = 3.14;
String name = "dri";
var inferred = 100;        # type inferred as int

# Ownership
Own<int[]> buf = new int[1024];
Own<int[]> other = move buf;   # explicit move — buf invalidated

# Traits
trait Printable { void print_info(); };
impl Printable for Point {
    void print_info() { print(this.x, this.y); };
};

# Generic with trait bound
void show<T implements Printable>(T item) {
    item.print_info();
};

# Error handling
Result<int> parse(String s) {
    if (length(s) == 0) { return Err("empty"); };
    return Ok(str.to_int(s));
};

match parse("42") {
    Ok(v)  => { print("value:", v); };
    Err(e) => { print("error:", e); };
};

# Module system
use math_utils;
use math_utils::fancy_sqrt;
```

---

### Documentation

| Language | Link |
|----------|------|
| 한국어 교육 문서 | [docs/ko/education/](docs/ko/education/) |
| Contributing | [docs/ko/CONTRIBUTING.md](docs/ko/CONTRIBUTING.md) |
| Roadmap | [docs/ko/ROADMAP.md](docs/ko/ROADMAP.md) |
| Security | [docs/ko/SECURITY.md](docs/ko/SECURITY.md) |

---

### Contributing

See [docs/ko/CONTRIBUTING.md](docs/ko/CONTRIBUTING.md) for contribution guidelines.

1. Open an issue first.
2. Branch from `main`.
3. Open a Pull Request.

---

### Roadmap

See [docs/ko/ROADMAP.md](docs/ko/ROADMAP.md) for the full roadmap.

| Milestone | Goal |
|-----------|------|
| v0.2 | C++17 transpiler foundation |
| v0.3 | Complete type system + ownership |
| v0.4 | HPC runtime (SIMD, parallel) |
| v0.5 | Auto-diff, compile-time features |
| v1.0 | Package manager (`drvpm`) + ecosystem |

---

### License

MIT License — see [LICENSE](LICENSE).

---

<br>

---

## 한국어

[English](#english) | [日本語](#日本語) | [中文](#中文)

### 목차

- [dri란?](#dri란)
- [핵심 특징](#핵심-특징)
- [빠른 시작](#빠른-시작)
- [컴파일](#컴파일)
- [CLI 옵션](#cli-옵션)
- [언어 개요](#언어-개요)
- [문서](#문서)
- [기여](#기여)
- [로드맵](#로드맵)
- [라이선스](#라이선스)

---

### dri란?

dri는 C++17로 트랜스파일되는 현대적 HPC 시스템 프로그래밍 언어입니다.  
과학 계산, 교육, 고성능 애플리케이션을 위해 설계되었습니다.

```dri
# hello.dri
print("안녕하세요, dri!");

int x = 10;
double pi = 3.14159;
print(`{x} * {pi} = {x as double * pi}`);

for (i in 0..5) {
    print(i);
};
```

---

### 핵심 특징

| 특징 | 설명 |
|------|------|
| **생산성** | Python 수준 문법 — 직관적, 최소 보일러플레이트 |
| **안전성** | 소유권 모델(`Own`, `Ref`, `Borrow`) + 타입 안전 |
| **성능** | 제로 코스트 추상화, C++17 트랜스파일 |
| **병렬** | `parallel for`, `simd for`, `async/await`, `spawn` |
| **수치 계산** | 내장 SIMD, `tensor` 연산, 자동 미분(`diff.*`) |
| **교육** | 명확한 오류 메시지, 직관적인 개념 |

---

### 빠른 시작

```dri
# fib.dri
int fib(int n) {
    if (n <= 1) { return n; };
    return fib(n - 1) + fib(n - 2);
};

print(fib(10));   # 55

# SIMD 벡터 덧셈
int N = 1024;
double[] a = new double[N];
double[] b = new double[N];
double[] c = new double[N];

simd for (i in 0..N) {
    c[i] = a[i] + b[i];
};

# 병렬 처리
list<String> files = [];
lst.push(files, "data1.csv");
lst.push(files, "data2.csv");

parallel for (f of files) {
    String content = io.read_file(f);
    print("로드됨:", f, length(content), "바이트");
};
```

---

### 컴파일

dri는 C++17로 트랜스파일 후 clang 또는 g++로 빌드합니다.

```
dri 소스  →  [dri 컴파일러]  →  C++17  →  [clang/g++]  →  바이너리
```

```bash
dri hello.dri --exe hello
./hello
```

빌드 모드:

```bash
dri main.dri --exe main --debug    # -O0, 범위 검사 활성화
dri main.dri --exe main --release  # -O3, -march=native, LTO
```

---

### CLI 옵션

```
dri <입력.dri> [옵션]
```

| 옵션 | 설명 |
|------|------|
| `--exe <파일>` | 실행 파일 생성 |
| `--cpp <파일>` | C++ 소스만 출력 |
| `--check` | 타입 검사만 수행, 코드 생성 없음 |
| `--debug` | 디버그 빌드 (`-O0`, 범위 검사) |
| `--release` | 릴리스 빌드 (`-O3`, LTO, native) |
| `--opt <0–3>` | 최적화 수준 수동 지정 |
| `--trace <파일>` | parallel 루프 추적 파일 생성 (Chrome DevTools) |
| `-D<플래그>` | `static_if` 조건용 플래그 정의 |

---

### 언어 개요

```dri
# 타입
int x = 42;
double pi = 3.14;
String name = "dri";
var inferred = 100;        # int로 추론 (지역 변수만)

# 소유권
Own<int[]> buf = new int[1024];
Own<int[]> other = move buf;   # 명시적 move — buf 무효화

# 트레이트
trait Printable { void print_info(); };
impl Printable for Point {
    void print_info() { print(this.x, this.y); };
};

# 트레이트 바운드 제네릭
void show<T implements Printable>(T item) {
    item.print_info();
};

# 예외 처리
Result<int> parse(String s) {
    if (length(s) == 0) { return Err("빈 문자열"); };
    return Ok(str.to_int(s));
};

match parse("42") {
    Ok(v)  => { print("값:", v); };
    Err(e) => { print("오류:", e); };
};

# 모듈 시스템
use math_utils;
use math_utils::fancy_sqrt;
```

---

### 문서

| 항목 | 링크 |
|------|------|
| 교육 문서 (한국어) | [docs/ko/education/](docs/ko/education/) |
| 기여 가이드 | [docs/ko/CONTRIBUTING.md](docs/ko/CONTRIBUTING.md) |
| 로드맵 | [docs/ko/ROADMAP.md](docs/ko/ROADMAP.md) |
| 보안 정책 | [docs/ko/SECURITY.md](docs/ko/SECURITY.md) |

---

### 기여

기여 방법은 [docs/ko/CONTRIBUTING.md](docs/ko/CONTRIBUTING.md)를 참고하세요.

1. 이슈를 먼저 열어 작업 내용을 공유합니다.
2. `main` 브랜치에서 분기합니다.
3. Pull Request를 생성합니다.

---

### 로드맵

전체 로드맵은 [docs/ko/ROADMAP.md](docs/ko/ROADMAP.md)를 참고하세요.

| 마일스톤 | 목표 |
|---------|------|
| v0.2 | C++17 트랜스파일러 기반 구축 |
| v0.3 | 타입 시스템 + 소유권 완성 |
| v0.4 | HPC 런타임 (SIMD, 병렬) |
| v0.5 | 자동 미분, 컴파일 타임 기능 |
| v1.0 | 패키지 관리자(`drvpm`) + 생태계 |

---

### 라이선스

MIT License — [LICENSE](LICENSE) 참고.

---

<br>

---

## 日本語

[English](#english) | [한국어](#한국어) | [中文](#中文)

### 目次

- [driとは](#driとは)
- [主な特徴](#主な特徴)
- [クイックスタート](#クイックスタート)
- [コンパイル](#コンパイル)
- [CLIオプション](#CLIオプション)
- [言語概要](#言語概要)
- [ドキュメント](#ドキュメント)
- [コントリビュート](#コントリビュート)
- [ロードマップ](#ロードマップ)
- [ライセンス](#ライセンス)

---

### driとは

driはC++17にトランスパイルされる現代的なHPCシステムプログラミング言語です。  
科学計算・教育・高性能アプリケーション向けに設計されています。

```dri
# hello.dri
print("こんにちは、dri!");

int x = 10;
double pi = 3.14159;
print(`{x} * {pi} = {x as double * pi}`);

for (i in 0..5) {
    print(i);
};
```

---

### 主な特徴

| 特徴 | 説明 |
|------|------|
| **生産性** | Pythonレベルの文法 — 読みやすく、ボイラープレートが少ない |
| **安全性** | 所有権モデル(`Own`, `Ref`, `Borrow`) + 型安全 |
| **性能** | ゼロコスト抽象化、C++17トランスパイル |
| **並列処理** | `parallel for`、`simd for`、`async/await`、`spawn` |
| **数値計算** | 組み込みSIMD、`tensor`演算、自動微分(`diff.*`) |
| **教育** | 明確なエラーメッセージ、直感的なコンセプト |

---

### クイックスタート

```dri
# fib.dri
int fib(int n) {
    if (n <= 1) { return n; };
    return fib(n - 1) + fib(n - 2);
};

print(fib(10));   # 55

# SIMDベクトル加算
int N = 1024;
double[] a = new double[N];
double[] b = new double[N];
double[] c = new double[N];

simd for (i in 0..N) {
    c[i] = a[i] + b[i];
};

# 並列処理
list<String> files = [];
lst.push(files, "data1.csv");
lst.push(files, "data2.csv");

parallel for (f of files) {
    String content = io.read_file(f);
    print("読込:", f, length(content), "バイト");
};
```

---

### コンパイル

driはC++17にトランスパイルし、clangまたはg++でビルドします。

```
driソース  →  [driコンパイラ]  →  C++17  →  [clang/g++]  →  バイナリ
```

```bash
dri hello.dri --exe hello
./hello
```

ビルドモード:

```bash
dri main.dri --exe main --debug    # -O0、境界チェック有効
dri main.dri --exe main --release  # -O3、-march=native、LTO
```

---

### CLIオプション

```
dri <入力.dri> [オプション]
```

| オプション | 説明 |
|-----------|------|
| `--exe <ファイル>` | 実行ファイルを生成 |
| `--cpp <ファイル>` | C++ソースのみ出力 |
| `--check` | 型検査のみ、コード生成なし |
| `--debug` | デバッグビルド(`-O0`、境界チェック) |
| `--release` | リリースビルド(`-O3`、LTO、native) |
| `--opt <0–3>` | 最適化レベルを手動指定 |
| `--trace <ファイル>` | 並列ループのトレースファイル生成 |
| `-D<フラグ>` | `static_if`条件用フラグ定義 |

---

### 言語概要

```dri
# 型
int x = 42;
var inferred = 100;   # int として推論 (ローカル変数のみ)

# 所有権
Own<int[]> buf = new int[1024];
Own<int[]> other = move buf;   # 明示的move — bufが無効化される

# トレイト
trait Printable { void print_info(); };
impl Printable for Point {
    void print_info() { print(this.x, this.y); };
};

# トレイト境界付きジェネリクス
void show<T implements Printable>(T item) {
    item.print_info();
};

# エラーハンドリング
Result<int> parse(String s) {
    if (length(s) == 0) { return Err("空文字"); };
    return Ok(str.to_int(s));
};

match parse("42") {
    Ok(v)  => { print("値:", v); };
    Err(e) => { print("エラー:", e); };
};

# モジュールシステム
use math_utils;
use math_utils::fancy_sqrt;
```

---

### ドキュメント

| 項目 | リンク |
|------|--------|
| 教育ドキュメント (韓国語) | [docs/ko/education/](docs/ko/education/) |
| コントリビュートガイド | [docs/ko/CONTRIBUTING.md](docs/ko/CONTRIBUTING.md) |
| ロードマップ | [docs/ko/ROADMAP.md](docs/ko/ROADMAP.md) |
| セキュリティポリシー | [docs/ko/SECURITY.md](docs/ko/SECURITY.md) |

---

### コントリビュート

コントリビュート方法は [docs/ko/CONTRIBUTING.md](docs/ko/CONTRIBUTING.md) を参照してください。

1. まずIssueを開いて作業内容を共有します。
2. `main` ブランチから分岐します。
3. Pull Requestを作成します。

---

### ロードマップ

| マイルストーン | 目標 |
|-------------|------|
| v0.2 | C++17トランスパイラ基盤 |
| v0.3 | 型システム + 所有権の完成 |
| v0.4 | HPCランタイム (SIMD、並列) |
| v0.5 | 自動微分、コンパイル時機能 |
| v1.0 | パッケージマネージャ(`drvpm`) + エコシステム |

---

### ライセンス

MIT License — [LICENSE](LICENSE) を参照。

---

<br>

---

## 中文

[English](#english) | [한국어](#한국어) | [日本語](#日本語)

### 目录

- [什么是dri](#什么是dri)
- [核心特性](#核心特性)
- [快速开始](#快速开始)
- [编译](#编译)
- [CLI选项](#CLI选项)
- [语言概述](#语言概述)
- [文档](#文档)
- [贡献](#贡献)
- [路线图](#路线图)
- [许可证](#许可证)

---

### 什么是dri

dri 是一门转译为 C++17 的现代 HPC 系统编程语言。  
专为科学计算、教育和高性能应用而设计。

```dri
# hello.dri
print("你好，dri!");

int x = 10;
double pi = 3.14159;
print(`{x} * {pi} = {x as double * pi}`);

for (i in 0..5) {
    print(i);
};
```

---

### 核心特性

| 特性 | 描述 |
|------|------|
| **生产力** | Python 级语法 — 可读性强，样板代码少 |
| **安全性** | 所有权模型(`Own`、`Ref`、`Borrow`) + 类型安全 |
| **性能** | 零成本抽象，转译为 C++17 |
| **并行** | `parallel for`、`simd for`、`async/await`、`spawn` |
| **数值计算** | 内置 SIMD、`tensor` 运算、自动微分(`diff.*`) |
| **教育** | 清晰的错误信息，直观的概念 |

---

### 快速开始

```dri
# fib.dri
int fib(int n) {
    if (n <= 1) { return n; };
    return fib(n - 1) + fib(n - 2);
};

print(fib(10));   # 55

# SIMD 向量加法
int N = 1024;
double[] a = new double[N];
double[] b = new double[N];
double[] c = new double[N];

simd for (i in 0..N) {
    c[i] = a[i] + b[i];
};

# 并行处理
list<String> files = [];
lst.push(files, "data1.csv");
lst.push(files, "data2.csv");

parallel for (f of files) {
    String content = io.read_file(f);
    print("已加载:", f, length(content), "字节");
};
```

---

### 编译

dri 转译为 C++17 后由 clang 或 g++ 构建。

```
dri 源码  →  [dri 编译器]  →  C++17  →  [clang/g++]  →  二进制
```

```bash
dri hello.dri --exe hello
./hello
```

构建模式:

```bash
dri main.dri --exe main --debug    # -O0，启用边界检查
dri main.dri --exe main --release  # -O3，-march=native，LTO
```

---

### CLI选项

```
dri <输入.dri> [选项]
```

| 选项 | 描述 |
|------|------|
| `--exe <文件>` | 生成可执行文件 |
| `--cpp <文件>` | 仅输出 C++ 源码 |
| `--check` | 仅类型检查，不生成代码 |
| `--debug` | 调试构建(`-O0`，边界检查) |
| `--release` | 发布构建(`-O3`，LTO，native) |
| `--opt <0–3>` | 手动指定优化级别 |
| `--trace <文件>` | 生成并行循环追踪文件 |
| `-D<标志>` | 为 `static_if` 定义编译时标志 |

---

### 语言概述

```dri
# 类型
int x = 42;
var inferred = 100;   # 推断为 int（仅限局部变量）

# 所有权
Own<int[]> buf = new int[1024];
Own<int[]> other = move buf;   # 显式 move — buf 失效

# Trait
trait Printable { void print_info(); };
impl Printable for Point {
    void print_info() { print(this.x, this.y); };
};

# 带 Trait 约束的泛型
void show<T implements Printable>(T item) {
    item.print_info();
};

# 错误处理
Result<int> parse(String s) {
    if (length(s) == 0) { return Err("空字符串"); };
    return Ok(str.to_int(s));
};

match parse("42") {
    Ok(v)  => { print("值:", v); };
    Err(e) => { print("错误:", e); };
};

# 模块系统
use math_utils;
use math_utils::fancy_sqrt;
```

---

### 文档

| 项目 | 链接 |
|------|------|
| 教育文档（韩语） | [docs/ko/education/](docs/ko/education/) |
| 贡献指南 | [docs/ko/CONTRIBUTING.md](docs/ko/CONTRIBUTING.md) |
| 路线图 | [docs/ko/ROADMAP.md](docs/ko/ROADMAP.md) |
| 安全政策 | [docs/ko/SECURITY.md](docs/ko/SECURITY.md) |

---

### 贡献

贡献方式请参见 [docs/ko/CONTRIBUTING.md](docs/ko/CONTRIBUTING.md)。

1. 先开一个 Issue 分享工作内容。
2. 从 `main` 分支创建分支。
3. 提交 Pull Request。

---

### 路线图

| 里程碑 | 目标 |
|--------|------|
| v0.2 | C++17 转译器基础 |
| v0.3 | 类型系统 + 所有权完善 |
| v0.4 | HPC 运行时（SIMD、并行） |
| v0.5 | 自动微分、编译时特性 |
| v1.0 | 包管理器(`drvpm`) + 生态系统 |

---

### 许可证

MIT License — 参见 [LICENSE](LICENSE)。

---
