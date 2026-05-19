# 例 7: コンパイル時メタプログラミング

> `compile_eval`、`static_if`、`dim` を組み合わせて
> 複雑な計算をコンパイル時に完了させ、実行時コストをゼロにする。

---

## 概要

- `compile_eval` : 関数を `constexpr` に変換 → コンパイル時に実行
- `static_if` : コンパイル時条件分岐 (バイナリ配布可能)
- `dim` + `compile_eval` : 単位変換定数をコンパイル時に計算
- `-D` フラグでビルドプロファイルを選択

---

## コード

```dri
module meta;

# ── 1. コンパイル時の数学関数 ────────────────────────────

compile_eval int fibonacci(int n) {
    if (n <= 1) { return n; }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

compile_eval int factorial(int n) {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}

compile_eval boolean is_prime(int n) {
    if (n < 2) { return false; }
    for (i in 2..n) {
        if (n % i == 0) { return false; }
    }
    return true;
}

# コンパイル時に計算 — 実行時には定数として使用
int fib20  = fibonacci(20);   # コンパイル時: 6765
int fac12  = factorial(12);   # コンパイル時: 479001600
boolean p7 = is_prime(7);     # コンパイル時: true

print(`fib(20) = ${fib20}`);
print(`12! = ${fac12}`);
print(`is_prime(7) = ${p7}`);

# ── 2. コンパイル時ルックアップテーブル ──────────────────

compile_eval int[] precompute_fibonacci_table(int n) {
    int[] table = lst.fill(n, 0);
    table[0] = 0;
    if (n > 1) { table[1] = 1; }
    for (i in 2..n) {
        table[i] = table[i-1] + table[i-2];
    }
    return table;
}

# コンパイル時に生成されたフィボナッチテーブル
int[] FIB_TABLE = precompute_fibonacci_table(30);
print(`fib(25) = ${FIB_TABLE[25]}`);  # O(1) ルックアップ

# ── 3. 物理単位のコンパイル時定数 ───────────────────────

dim Meter   = "m";
dim Foot    = "ft";
dim Celsius = "°C";
dim Kelvin  = "K";

# 単位変換定数 (コンパイル時に計算)
compile_eval double feet_to_meters(double feet) {
    return feet * 0.3048;
}

compile_eval double celsius_to_kelvin(double c) {
    return c + 273.15;
}

compile_eval double meters_to_feet(double meters) {
    return meters / 0.3048;
}

double EMPIRE_STATE_METERS = feet_to_meters(1454.0);  # コンパイル時
double BOILING_KELVIN      = celsius_to_kelvin(100.0); # コンパイル時
double MARATHON_FEET       = meters_to_feet(42195.0);  # コンパイル時

print(`エンパイアステートビル: ${EMPIRE_STATE_METERS} m`);
print(`沸点: ${BOILING_KELVIN} K`);
print(`マラソン: ${MARATHON_FEET} ft`);

# ── 4. static_if — ビルドプロファイル別コード ────────────

static_if (DEBUG) {
    # デバッグビルド: 詳細ログ、境界チェック
    print("[DEBUG] デバッグモードでビルドされました");

    void checked_get(int[] arr, int idx) {
        if (idx < 0 or idx >= lst.length(arr)) {
            throw `インデックス範囲外: ${idx} / ${lst.length(arr)}`;
        }
    }

} else {
    static_if (RELEASE) {
        # リリースビルド: 最適化、ログ除去
        print("[RELEASE] リリースモード");

    } else {
        # デフォルトビルド
        print("[DEFAULT] デフォルトビルド");
    }
}

# プラットフォーム別の分岐
static_if (WINDOWS) {
    String PATH_SEP = "\\";
    String CONFIG_DIR = "%APPDATA%\\myapp";
} else {
    static_if (MACOS) {
        String PATH_SEP = "/";
        String CONFIG_DIR = "~/Library/Application Support/myapp";
    } else {
        String PATH_SEP = "/";
        String CONFIG_DIR = "~/.config/myapp";
    }
}

# ── 5. 型別コンパイル時特殊化 ────────────────────────────

# T に応じて異なるアルゴリズムを選択
compile_eval boolean is_floating_point_type(String type_name) {
    return type_name == "double" or type_name == "float";
}

void compute<T>(T a, T b) {
    static_if (EXACT_ARITHMETIC) {
        # 整数精密計算
        print(`整数結果: ${a + b}`);
    } else {
        # 浮動小数点計算
        print(`実数結果: ${a + b}`);
    }
}

# ── 6. コンパイル時エラトステネスの篩 ────────────────────

compile_eval int[] sieve_of_eratosthenes(int limit) {
    boolean[] is_prime_arr = lst.fill(limit + 1, true);
    is_prime_arr[0] = false;
    if (limit > 0) { is_prime_arr[1] = false; }

    for (i in 2..limit) {
        if (is_prime_arr[i]) {
            int j = i * i;
            while (j <= limit) {
                is_prime_arr[j] = false;
                j += i;
            }
        }
    }

    int[] primes = [];
    for (i in 2..limit) {
        if (is_prime_arr[i]) {
            lst.push(primes, i);
        }
    }
    return primes;
}

# 100 以下のすべての素数をコンパイル時に計算
int[] PRIMES_100 = sieve_of_eratosthenes(100);

print(`100 以下の素数の個数: ${lst.length(PRIMES_100)}`);
print("最初の 10 個:");
for (p of lst.take(PRIMES_100, 10)) {
    print(p);
}

# ── 7. 行列サイズのコンパイル時最適化 ────────────────────

compile_eval int next_power_of_2(int n) {
    int p = 1;
    while (p < n) { p = p * 2; }
    return p;
}

int OPTIMAL_SIZE = next_power_of_2(500);   # コンパイル時: 512
print(`最適な行列サイズ (500→): ${OPTIMAL_SIZE}`);
```

---

## コンパイル時 vs 実行時の比較

| 計算 | なし | `compile_eval` あり |
|------|------|--------------------|
| `fibonacci(20)` | 実行時に 6765 回の再帰 | コンパイル時に 1 回 → 定数 |
| `sieve(100)` | 毎回実行のたびに計算 | コンパイル時 → 配列定数 |
| `static_if (DEBUG)` | 実行時 `if` | コンパイル時に完全に除去/組み込み |

---

## ビルドコマンド

```bash
# デバッグビルド
dri main.dri --exe app -DDEBUG

# リリースビルド
dri main.dri --exe app --release -DRELEASE

# Windows ビルド
dri main.dri --exe app -DWINDOWS

# 複数のフラグを同時に
dri main.dri --exe app --release -DRELEASE -DWINDOWS
```

---

## 学習ポイント

- `compile_eval` = C++ の `constexpr` への自動変換
- `static_if (FLAG)` = バイナリからの条件付きコードの完全な除去/組み込み
- ルックアップテーブルパターン: O(n) の前処理 → O(1) の実行時アクセス
- `dim` + `compile_eval` = 単位変換エラーをコンパイル時にブロック
