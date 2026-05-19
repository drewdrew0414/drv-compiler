# 制御構文

> すべての制御構造は波括弧`{ }`ブロックを使用し、文全体は`;`で終わります。

---

## if / else if / else文

```
if (条件) { 本文 };
if (条件) { 本文 } else { 本文 };
if (条件) { 本文 } else if (条件2) { 本文 } else { 本文 };
```

```dri
int score = 85;

if (score >= 90) {
    print("A");
} else if (score >= 80) {
    print("B");
} else if (score >= 70) {
    print("C");
} else {
    print("F");
};
# 出力: B
```

```dri
int x = 15;
if (x > 10 and x < 20) {
    print("10と20の間");
};

boolean logged_in = true;
boolean is_admin = false;
if (logged_in and not is_admin) {
    print("一般ユーザー");
};
```

---

## for文

```
for (変数 in 開始..終了) { 本文 };
```

`開始..終了`は開始以上、終了**未満**(終了は含まれません)。

```dri
# 0, 1, 2, 3, 4
for (i in 0..5) {
    print(i);
};

# 合計の計算
int sum = 0;
for (i in 1..101) {
    sum += i;
};
print("1~100の合計:", sum);   # 5050
```

### for — コレクションの反復

```
for (変数 of コレクション) { 本文 };
```

```dri
int[] numbers = [10, 20, 30, 40, 50];
for (n of numbers) {
    print(n);
};
```

### for — Cスタイルの明示的な反復

```
for (型 変数 = 初期値; 条件; 変数++) { 本文 };
```

```dri
for (int i = 0; i < 10; i++) {
    print(i);
};

for (int i = 10; i > 0; i--) {
    print(i);
};

for (int i = 0; i < 100; i += 5) {
    print(i);
};
```

---

## while文

```
while (条件) { 本文 };
```

```dri
int count = 0;
while (count < 5) {
    print(count);
    count += 1;
};

# 無限ループ
int n = 1;
while (true) {
    if (n > 10) {
        break;
    };
    print(n);
    n += 1;
};
```

---

---

## break / continue / pass

```dri
# break — ループから抜ける
for (i in 0..10) {
    if (i == 5) {
        break;
    };
    print(i);
};
# 出力: 0 1 2 3 4

# continue — 現在の反復をスキップ
for (i in 0..10) {
    if (i % 2 == 0) {
        continue;
    };
    print(i);
};
# 出力: 1 3 5 7 9

# pass — 何もしない(空ブロックの代用)
for (i in 0..5) {
    pass;
};
```

---

## switch

```
switch (式) {
    case (値) -> { 本文 };
    case (値2) -> { 本文 };
};
```

```dri
int day = 3;
switch (day) {
    case (1) -> { print("月曜日"); };
    case (2) -> { print("火曜日"); };
    case (3) -> { print("水曜日"); };
    case (4) -> { print("木曜日"); };
    case (5) -> { print("金曜日"); };
    case default -> { print("休日"); };
};
# 出力: 水曜日
```

### 文字列switch:

```dri
String cmd = "quit";
switch (cmd) {
    case ("start") -> { print("開始"); };
    case ("stop")  -> { print("停止"); };
    case ("quit")  -> { print("終了"); };
};
```

---

## match — パターンマッチング

`Option<T>`、`Result<T>`の値を分解する際に使用します。

```
match 式 {
    パターン => { 本文 };
    パターン => { 本文 };
};
```

パターンの種類: `Some(変数)`、`None`、`Ok(変数)`、`Err(変数)`、`_`(ワイルドカード)、リテラル

```dri
Option<int> maybe = Some(42);
match maybe {
    Some(v) => { print("値:", v); };
    None    => { print("無し"); };
};
# 出力: 値: 42

Result<int> res = Ok(100);
match res {
    Ok(v)  => { print("成功:", v); };
    Err(e) => { print("失敗:", e); };
};
```

### ワイルドカードパターン:

```dri
match maybe {
    Some(v) => { print("有り:", v); };
    _       => { print("無し、または別のパターン"); };
};
```

### リテラルパターン:

```dri
int code = 404;
match code {
    200 => { print("OK"); };
    404 => { print("Not Found"); };
    500 => { print("Server Error"); };
    _   => { print("その他:", code); };
};
```

---

## parallel for — 並列反復

`for`の前に接頭辞を付けて実行方式を変更します。

```
parallel for (変数 of コレクション) { 本文 };
parallel for (変数 in 開始..終了) { 本文 };
```

ワークスティーリングのスレッドプールに基づいて、各反復を並列実行します。

```dri
int[] data = [1, 2, 3, 4, 5, 6, 7, 8];

parallel for (item of data) {
    # 各反復が異なるスレッドで実行される可能性がある
    print("処理:", item);
};
```

`simd for`と組み合わせると、並列とベクトル化を同時に適用できます:

```dri
parallel simd for (i in 0..N) {
    result[i] = a[i] * b[i] + c[i];
};
```

---

## simd for — SIMDベクトル化反復

```
simd for (変数 in 開始..終了) { 本文 };
```

```dri
int N = 1024;
double[] a = new double[N];
double[] b = new double[N];
double[] c = new double[N];

simd for (i in 0..N) {
    c[i] = a[i] + b[i];
};
```

### where / otherwise — SIMDマスク条件

`simd for`の内部で通常の`if/else`を使うと、分岐予測が崩れSIMDベクトル化が解除されます。
`where`/`otherwise`は、**AVX-512 Mask Register**や`vblendv`などのハードウェアマスク命令へ変換されます。

```
simd for (変数 in 範囲) {
    where (マスク条件) {
        本文_真
    } otherwise {
        本文_偽
    };
};
```

```dri
# 条件分岐なしでSIMDレーン全体をベクトルマスクで処理
simd for (i in 0..N) {
    where (x[i] > 0.0) {
        z[i] = sqrt(x[i]);
    } otherwise {
        z[i] = 0.0;
    };
};
# C++: z[i] = _mm512_mask_sqrt_pd(zero, mask, x[i]) の形に変換
```

複数マスクのネスト:

```dri
simd for (i in 0..N) {
    where (a[i] > 0.0 and b[i] < 1.0) {
        c[i] = a[i] * b[i];
    } otherwise {
        c[i] = 0.0;
    };
};
```

> `where`/`otherwise`は`simd for`の内部でのみ有効です。
> 通常の`for`の内部では通常の`if/else`を使用してください。

---

## exists — 変数存在確認

```
exists (変数名) { 本文 } else { 本文 };
```

```dri
String data;
exists (data) {
    print("data 存在:", data);
} else {
    print("data 無し");
};
```

---
