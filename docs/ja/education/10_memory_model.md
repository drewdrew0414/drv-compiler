# メモリモデル
> dri は Rust にインスパイアされた所有権・借用モデルを提供します。`Own`、`Ref`、`Borrow`、`@region` でメモリを制御します。

---

## 所有権タイプのまとめ

| タイプ | C++ 対応 | 所有権 | 解放 | 同時アクセス |
|------|----------|--------|------|----------|
| `Own<T>` | `unique_ptr<T>` | 単独 (move 可能) | 自動 | 単一スレッド |
| `Ref<T>` | `shared_ptr<T>` | 共有 | カウント 0 で自動 | 不変共有可能 |
| `Borrow<T>` | `const T&` | なし | 解放しない | 同時読み取り可能 |
| `mut Borrow<T>` | `T&` | なし | 解放しない | 独占書き込み |
| `ref T` | `T&` | なし | 解放しない | エイリアス |
| `@region` | arena | 領域全体 | ブロック終了時に一括 | 脱出禁止 |

> `Ref<T>`、`Own<T>`、`Borrow<T>` の基本的な宣言構文は [[02_basics.md](02_basics.md)] を参照してください。

---

## Ref\<T\> — 共有所有権 (shared_ptr)

複数の変数が同じオブジェクトを共有します。null を許容します。

```dri
Ref<String> a = new String();
Ref<MyClass> obj = new MyClass();

# null 許容
Ref<String> maybe = null;
if (maybe != null) {
    print(maybe);
};
```

連結リストの例:

```dri
class Node {
    public int val;
    public Ref<Node> next;
};

Ref<Node> head = new Node();
head.val = 1;
head.next = new Node();
head.next.val = 2;
head.next.next = null;

Ref<Node> cur = head;
while (cur != null) {
    print(cur.val);
    cur = cur.next;
};
```

---

## Own\<T\> — 単独所有権 (unique_ptr)

一度に一つの変数だけが所有します。所有権の移動は必ず **`move` キーワードを明示** する必要があります。
`move` なしで `Own<T>` を代入すると **コンパイルエラー** です — 暗黙的な移動は許可されません。

```dri
Own<int[]> buf = new int[1024];

# 明示的な move — 所有権の移動
Own<int[]> owner2 = move buf;
# 以降 buf は無効化 (invalidated) — アクセス不可

print(buf[0]);   # コンパイルエラー: 'buf' used after move
```

### move のルール

| 状況 | 動作 |
|------|------|
| `move` を明示 | 所有権が移動し、元の変数は無効化 |
| `move` なしで `Own<T>` を代入 | **コンパイルエラー** |
| 無効化された変数へのアクセス | **コンパイルエラー** |
| `Ref<T>` / `Borrow<T>` への代入 | `move` 不要 (共有/借用) |

```dri
# エラー例
Own<int[]> a = new int[64];
Own<int[]> b = a;         # エラー: move キーワードなしで Own<T> を代入不可
Own<int[]> c = move a;    # OK
print(a[0]);              # エラー: 'a' used after move
```

関数への所有権移動:

```dri
void consume(Own<int[]> data) {
    data[0] = 42;
    print(data[0]);
};

Own<int[]> arr = new int[512];
consume(move arr);         # arr の所有権が consume に移動
consume(move arr);         # エラー: 'arr' used after move
```

関数から所有権を返す (取り戻す):

```dri
Own<int[]> process(Own<int[]> data) {
    data[0] = 99;
    return move data;   # 所有権を返す
};

Own<int[]> buf = new int[128];
buf = process(move buf);   # 渡してから受け取る — buf が再び有効
print(buf[0]);             # 99
```

### コンパイラエラーメッセージ規約

| 状況 | エラーメッセージ |
|------|-----------|
| `move` なしで `Own<T>` を代入 | `error: cannot copy 'Own<T>' — use 'move' to transfer ownership` |
| 無効化後のアクセス | `error: 'X' used after move at line N` |
| `move` された変数の再利用 | `error: value moved into 'Y', 'X' is no longer valid` |

---

## Borrow\<T\> — 非所有参照 (const T&)

コピーなしで読み取り専用アクセスします。所有権は移動しません。

```dri
void print_buf(Borrow<int[]> buf) {
    print(buf[0], buf[1]);
};

Own<int[]> data = new int[4];
data[0] = 10;
data[1] = 20;

print_buf(data);    # 10 20
# data の所有権はそのまま維持
```

---

## mut Borrow\<T\> — 可変独占借用 (T&, exclusive)

読み取り専用の `Borrow<T>` と異なり、**可変借用は一つだけ** が同時に存在できます。
このルールにより、`parallel for` などの並列コンテキストで **データレース (Data Race) をコンパイル時に防ぎます**。

### 借用ルール (Read-Write Lock の哲学)

| 状態 | 許可 |
|------|------|
| `Borrow<T>` 不変借用を複数同時に保持 | OK |
| `mut Borrow<T>` 可変借用を一つだけ保持 | OK |
| 可変借用中に別の不変/可変借用を同時保持 | コンパイルエラー |

```dri
Own<int[]> data = new int[1024];

void read_only(Borrow<int[]> b) { print(b[0]); };
void write(mut Borrow<int[]> b) { b[0] = 99; };

read_only(data);    # OK
read_only(data);    # OK — 不変借用は複数許可


write(data);        # OK
write(data);        # OK — 順次実行なので同時可変借用ではない

# コンパイルエラー例 — 可変借用中に不変借用は不可
mut Borrow<int[]> w = data;
Borrow<int[]> r = data;   # エラー: w が生きている間は r を作成できない
```

### `parallel for` の安全性

```dri
int[] arr = [1, 2, 3, 4, 5];

# 安全: 各スレッドが独立した要素を不変で読む
parallel for (item of arr) {
    print(item * 2);
};

# コンパイルエラー: ループ内で arr 自体を変更しようとすると可変借用の競合
parallel for (item of arr) {
    arr[0] = 0;   # エラー: arr が並列に可変借用されている
};
```

---

## ref — 参照変数 (T&)

```dri
int x = 10;
ref int y = x;   # y は x のエイリアス
y = 20;          # x も 20 になる
print(x);        # 20
```

---

## @region — アリーナ確保

```
@region 名前 {
    本体
}
```

ブロック内で確保されたすべてのメモリは、ブロック終了時に一括解放されます。

```dri
@region scratch {
    int[] temp = [0, 0, 0, 0, 0, 0, 0, 0];
    double[] workspace = [1.0, 2.0, 3.0, 4.0];

    for (i in 0..8) {
        temp[i] = i * 2;
    };

    int s = 0;
    for (v of temp) {
        s += v;
    };
    print("合計:", s);
    # ブロック終了: temp, workspace を一括解放
}
```

### ネストされた region

```dri
@region outer {
    int[] big_buf = new int[10000];

    @region inner {
        int[] small_tmp = new int[64];
        # 演算を実行
        # inner 終了: small_tmp を即時解放
    }
    # big_buf はまだ有効
    # outer 終了: big_buf を解放
}
```

### HPC パターン — 反復ごとの一時確保

```dri
for (iter in 0..1000) {
    @region frame {
        double[] grad = new double[1024];
        double[] update = new double[1024];

        simd for (i in 0..1024) {
            grad[i] = compute_grad(i, iter);
            update[i] = grad[i] * 0.01;
        };
        # frame 終了: 反復ごとにヒープの断片化なく一括解放
    }
};
```

### @region 脱出解析 (Escape Analysis)

`@region` ブロック内部で確保されたメモリ、または `io.mmap_read` で取得したファイルマッピングが
**ブロック外の変数に代入 (脱出) されるとコンパイルエラー** になります。
ブロック終了時にメモリが解放されるため、外部参照は Use-After-Free 脆弱性を引き起こします。

```dri
# エラー例 — mmap の結果を region 外へ脱出させる
Ref<var> escaped;

@region frame {
    var mapped = io.mmap_read("huge.bin");
    escaped = mapped;   # コンパイルエラー: mapped が @region frame の外へ脱出
}
# この時点で frame 終了 → mapped 解放済み
# escaped は解放済みメモリを指すダングリングポインタ
print(escaped);   # Use-After-Free
```

```dri
# 正しいパターン — ブロック内でのみ使用
@region frame {
    var mapped = io.mmap_read("huge.bin");
    process(mapped);          # ブロック内でのみ参照
    double result = sum(mapped);
    print("エネルギー:", result);
    # frame 終了: mapped が安全に解放
}
```

> 脱出解析はコンパイラが自動実行します。脱出検出時のエラーメッセージ:
> `error: 'mapped' escapes @region 'frame' — use-after-free would occur`

---

## @stack — スタック強制確保

ヒープ確保のオーバーヘッドなしに、配列をスタックに置きます。

```dri
@stack
int[] local_buf = [0, 0, 0, 0, 0, 0, 0, 0];
# ヒープ確保のオーバーヘッドなし
```

---

## @heap — ヒープ明示確保

```dri
@heap
int[] large = new int[10000000];
# 明示的にヒープに確保されていることをドキュメント化
```

---

## @align — SIMD/NUMA メモリアライメント

SIMD 命令 (`simd for`、`simd.fmadd`) は、データが特定のバイト境界にアライメントされているときに最高性能を発揮します。
アライメントされていないメモリ (Misaligned Access) は性能低下やクラッシュを引き起こします。

| SIMD 命令 | 必要なアライメント |
|------------|----------|
| SSE2 | 16 バイト |
| AVX2 | 32 バイト |
| AVX-512 | 64 バイト |

```dri
# AVX-512 最適化のため 64 バイトアライメントを強制
@align(64)
double[] simd_buf = new double[1000000];

# AVX2 用 32 バイトアライメント構造体
@align(32)
class Vector3D {
    public double x;
    public double y;
    public double z;
};
```

> C++ トランスパイル時に `alignas(N)` または `__attribute__((aligned(N)))` に変換されます。

---

## compile_eval — コンパイル時実行

単純な定数計算:

```dri
int BUF_SIZE = compile_eval(1024 * 1024);
double PI_4 = compile_eval(3.14159 / 4.0);
print(BUF_SIZE);   # 1048576
```

### compile_eval 関数ブロック — ルックアップテーブル生成

`compile_eval` アノテーションを関数に付けると、コンパイル時とランタイムの両方で実行可能な関数になります。
`const` 変数に代入するとき **コンパイル時に実行** され、結果がバイナリのデータセクションに定数として埋め込まれます。
C++17 の `constexpr` を完全にラップします。

```dri
compile_eval double[] generate_sin_table() {
    mut double[] table = new double[360];
    for (i in 0..360) {
        table[i] = math.sin(i as double * 0.017453);   # 度 → ラジアン
    };
    return table;
};

# コンパイル時にテーブル生成 → バイナリに定数として埋め込まれる (ランタイムオーバーヘッド 0)
const double[] sin_table = generate_sin_table();

# ランタイムで即時参照
print(sin_table[90]);   # ~1.0
```

```dri
compile_eval int[] generate_primes(int limit) {
    mut int[] primes = [];
    for (n in 2..limit) {
        boolean is_prime = true;
        for (p of primes) {
            if (p * p > n) { break; };
            if (n % p == 0) { is_prime = false; break; };
        };
        if (is_prime) { lst.push(primes, n); };
    };
    return primes;
};

const int[] small_primes = generate_primes(1000);   # コンパイル時計算
```

---

## エフェクトアノテーション

関数がどのような副作用を持つかを明示します。

```dri
@alloc
int[] make_buf(int n) {
    return new int[n];
};

@io
void read_line() {
    String s;
    input(s);
};

@threadsafe
void increment() {
    counter += 1;
};
```

---

## atomic<T> — アトミック変数

```dri
atomic<int>  counter = 0;      # → std::atomic<int32_t>
atomic<long> flag    = 0L;     # → std::atomic<int64_t>
```

### atomic<String> の注意点

`atomic<String>` は内部的に **ポインタ** をアトミックに交換します。文字列の内容そのものはアトミックに更新されません。

```dri
atomic<String> msg = "hello";

spawn {
    sys.sync.atomic_store(msg, "world");   # ポインタのみアトミック交換
};

String current = sys.sync.atomic_load(msg);
```

> **コンパイラ警告**: `atomic<String>` 宣言時に自動で警告を出力します。
> 完全な文字列のアトミック性が必要な場合は `Ref<String>` + mutex の利用を推奨します。

---

## 増分ビルド (Incremental Build)

大規模プロジェクトで変更されていないファイルの再コンパイルをスキップします。

```bash
dri main.dri --exe app --incremental          # .dri_cache/ にハッシュを保存
dri main.dri --exe app --incremental --cache-dir /tmp/dri-cache
```

**動作方式**:
1. ファイル更新時刻 (mtime) を比較 → 変更が確実なら即座に再コンパイル
2. mtime が同一 → CRC32 ハッシュを比較 → 内容が同じならキャッシュヒット (再コンパイルをスキップ)
3. 成功時に `.dri_cache/hashes.txt` にハッシュを記録

```
.dri_cache/
└── hashes.txt   # パス + mtime + CRC32
```
