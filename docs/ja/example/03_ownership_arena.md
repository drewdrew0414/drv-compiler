# 例 3: 所有権システム + アリーナアロケータ

> Own<T>/Ref<T>/Borrow<T> の違いを実践的なパターンで示す。
> @region アリーナでフラグメンテーションのない高性能メモリ管理を実装する。

---

## 概要

- `Own<T>` : 排他的所有権 (ムーブ後は元の変数が無効になる)
- `Ref<T>` : 共有所有権 (参照カウント)
- `Borrow<T>` : 読み取り専用の一時参照
- `mut Borrow<T>` : 排他的可変参照
- `@region` : スコープベースのアリーナ — ブロック終了時に一括解放

---

## コード

```dri
module arena_example;

# ── 1. 基本的な所有権 ──────────────────────────────────────────

class Node {
    int value;
    String label;
}

void demo_ownership() {
    # Own<T>: 排他的所有 — ムーブ後は元の変数を使用不可
    Own<Node> a = new Node();
    a.value = 42;
    a.label = "first";

    Own<Node> b = move a;   # 所有権を移転
    print(`b.value = ${b.value}`);
    # a は無効になった — コンパイラが検出する

    # Ref<T>: 共有所有 — 複数の場所から参照可能
    Ref<Node> r1 = new Node();
    r1.value = 100;
    Ref<Node> r2 = r1;      # 参照カウントが増加
    r2.value = 200;         # r1 と r2 が同じオブジェクトを共有
    print(`r1.value = ${r1.value}`);  # 200 を出力
}

# ── 2. Borrow — 一時読み取り参照 ──────────────────────────────

double sum_values(Borrow<list<int>> arr) {
    # Borrow<T>: 読み取り専用、所有権の移転なし
    double s = 0.0;
    for (x of arr) { s += x; }
    return s;
}

void scale_inplace(mut Borrow<list<double>> arr, double factor) {
    # mut Borrow<T>: 排他的可変参照
    for (i in 0..lst.length(arr)) {
        arr[i] = arr[i] * factor;
    }
}

# ── 3. @region アリーナアロケーション ────────────────────────

class Particle {
    double x;
    double y;
    double vx;
    double vy;
    double mass;
}

void simulate_frame(int n_particles) {
    @region frame {
        # このブロック内で確保されたすべてのメモリは
        # ブロック終了時に一括解放される (ヒープフラグメンテーションなし)
        list<Particle> particles = [];
        double[] forces_x = lst.fill(n_particles, 0.0);
        double[] forces_y = lst.fill(n_particles, 0.0);
        double[] temp_buf = lst.fill(n_particles * 4, 0.0);

        # パーティクルを初期化
        for (i in 0..n_particles) {
            Particle p;
            p.x = i * 0.1;
            p.y = 0.0;
            p.vx = 0.0;
            p.vy = 0.0;
            p.mass = 1.0;
            lst.push(particles, p);
        }

        # 力を計算
        for (i in 0..n_particles) {
            forces_x[i] = -particles[i].x * 9.8;  # 単純な復元力
            forces_y[i] = -9.8;
        }

        # 位置を更新
        for (i in 0..n_particles) {
            particles[i].vx += forces_x[i] * 0.016;
            particles[i].vy += forces_y[i] * 0.016;
            particles[i].x  += particles[i].vx * 0.016;
            particles[i].y  += particles[i].vy * 0.016;
        }

        print(`フレーム完了: ${n_particles} 個のパーティクルを処理`);
        # ← ここで particles、forces_x、forces_y、temp_buf がすべて自動解放
    }
    print("アリーナ解放完了");
}

# ── 4. ネストされたアリーナ ───────────────────────────────────

void nested_arenas() {
    @region outer {
        double[] persistent = lst.fill(1000, 1.0);

        for (frame in 0..60) {
            @region inner {
                # フレームごとの一時データ — 毎フレーム再利用
                double[] scratch = lst.fill(10000, 0.0);
                double[] indices = lst.fill(5000, 0.0);

                # 処理ロジック...
                for (i in 0..1000) {
                    scratch[i] = persistent[i] * frame;
                }
                # inner 終了: scratch、indices が解放
            }
            # persistent は outer スコープにあるため保持される
        }
        # outer 終了: persistent が解放
    }
}

# ── 5. エスケープ解析の例 ─────────────────────────────────────

# @region エスケープ解析: アリーナ外にポインタが漏れるのを防ぐ
void escape_analysis_demo() {
    String leaked_ref;  # @region の外で宣言

    @region temp {
        String local_str = "アリーナ内部の文字列";
        # コンパイラ警告: local_str が @region の外にエスケープ
        # leaked_ref = local_str;  ← この行はコンパイルエラー
        print(local_str);  # OK: アリーナ内部での使用
    }
    # leaked_ref を使うと use-after-free が発生 — dri がブロックする
}

# 実行
demo_ownership();

list<int> nums = [1, 2, 3, 4, 5];
print(`合計: ${sum_values(nums)}`);

list<double> vals = [1.0, 2.0, 3.0];
scale_inplace(vals, 2.0);
for (v of vals) { print(v); }

simulate_frame(1000);
nested_arenas();
escape_analysis_demo();
```

---

## 所有権ルールのまとめ

```
Own<T>  ──→  move  ──→  Own<T>     # ムーブのみ、コピー不可
Ref<T>  ──→  共有  ──→  Ref<T>     # 参照カウント
Borrow<T>        ←── 読み取り専用の一時参照
mut Borrow<T>    ←── 排他的可変参照 (同時に 1 つのみ)
```

| 型 | C++ 変換 | コスト | 用途 |
|----|----------|-------|------|
| `Own<T>` | `unique_ptr<T>` | ゼロコスト | 唯一の所有者 |
| `Ref<T>` | `shared_ptr<T>` | 参照カウント | 共有所有 |
| `Borrow<T>` | `const T&` | ゼロコスト | 読み取り専用 |
| `mut Borrow<T>` | `T&` | ゼロコスト | 可変参照 |

---

## 学習ポイント

- `move` キーワードなしに `Own<T>` のコピーを試みる → コンパイルエラー
- `@region` 内部のポインタが外部にエスケープ → コンパイルエラー
- `mut Borrow<T>` は同時に 1 つしか存在できない (データ競合を防止)
