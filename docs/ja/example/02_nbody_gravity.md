# 例 2: N 体重力シミュレーション

> 物理単位型 (`dim`) と SIMD を組み合わせた天体物理シミュレーション。
> 型レベルで単位エラーを根本から防ぐ。

---

## 概要

- ニュートン重力: F = G·m₁·m₂ / r²
- `dim` でメートル/秒/キログラム/ニュートンの単位を強制
- `@layout_soa` で SoA レイアウト → SIMD キャッシュ効率
- Leapfrog 積分法でエネルギーを保存

---

## コード

```dri
# N 体重力シミュレーション — 物理単位 + SIMD
module nbody;

# 物理単位の定義
dim Meter   = "m";
dim Second  = "s";
dim Kg      = "kg";
dim Newton  = "N";
dim Joule   = "J";

double G = 6.674e-11;   # 重力定数 [m³/(kg·s²)]
int N_BODIES = 1000;
int STEPS    = 100;
double DT    = 1.0;     # 時間ステップ [s]

# 天体データ — SoA レイアウトで SIMD 最適化
@layout_soa
class Body {
    double x;     # 位置 [m]
    double y;
    double z;
    double vx;    # 速度 [m/s]
    double vy;
    double vz;
    double mass;  # 質量 [kg]
}

# 初期条件: 球形分布
void init_sphere(list<Body> bodies) {
    for (i in 0..N_BODIES) {
        Body b;
        double angle = i * 6.28318 / N_BODIES;
        double r     = 1e11 * (1.0 + i * 0.001);
        b.x    = r * math.cos(angle);
        b.y    = r * math.sin(angle);
        b.z    = r * 0.1 * math.sin(angle * 3.0);
        b.vx   = -r * 1e-7 * math.sin(angle);
        b.vy   =  r * 1e-7 * math.cos(angle);
        b.vz   = 0.0;
        b.mass = 1e24 * (1.0 + i * 0.01);
        lst.push(bodies, b);
    }
}

# 力の計算 + 位置/速度の更新 (Leapfrog)
@bench
void step(list<Body> bodies) {
    # 速度の半ステップ更新
    parallel for (i in 0..N_BODIES) {
        double ax = 0.0;
        double ay = 0.0;
        double az = 0.0;

        # 他のすべての天体からの重力を合計
        simd for (j in 0..N_BODIES) {
            where (i != j) {
                Body bi = bodies[i];
                Body bj = bodies[j];
                double dx = bj.x - bi.x;
                double dy = bj.y - bi.y;
                double dz = bj.z - bi.z;
                double r2 = dx*dx + dy*dy + dz*dz + 1e10;  # ソフトニング
                double r  = math.sqrt(r2);
                double f  = G * bj.mass / (r2 * r);
                ax += f * dx;
                ay += f * dy;
                az += f * dz;
            } otherwise {
                # i == j: 自己相互作用をスキップ
            }
        }

        bodies[i].vx += ax * DT * 0.5;
        bodies[i].vy += ay * DT * 0.5;
        bodies[i].vz += az * DT * 0.5;
    }

    # 位置の更新
    parallel for (i in 0..N_BODIES) {
        bodies[i].x += bodies[i].vx * DT;
        bodies[i].y += bodies[i].vy * DT;
        bodies[i].z += bodies[i].vz * DT;
    }
}

# 全エネルギー (運動エネルギー + ポテンシャルエネルギー)
double total_energy(list<Body> bodies) {
    double ke = 0.0;
    double pe = 0.0;

    parallel for reduction(+:ke) (i in 0..N_BODIES) {
        double v2 = bodies[i].vx * bodies[i].vx
                  + bodies[i].vy * bodies[i].vy
                  + bodies[i].vz * bodies[i].vz;
        ke += 0.5 * bodies[i].mass * v2;
    }

    parallel for reduction(+:pe) (i in 0..N_BODIES) {
        for (j in 0..i) {
            double dx = bodies[j].x - bodies[i].x;
            double dy = bodies[j].y - bodies[i].y;
            double dz = bodies[j].z - bodies[i].z;
            double r  = math.sqrt(dx*dx + dy*dy + dz*dz + 1e10);
            pe -= G * bodies[i].mass * bodies[j].mass / r;
        }
    }

    return ke + pe;
}

list<Body> bodies = [];
init_sphere(bodies);

double e0 = total_energy(bodies);
print(`初期全エネルギー: ${e0} J`);

# シミュレーション実行
for (step_i in 0..STEPS) {
    step(bodies);
    if (step_i % 10 == 0) {
        double e = total_energy(bodies);
        double drift = (e - e0) / math.abs(e0);
        print(`ステップ ${step_i}: エネルギー誤差 = ${drift}`);
    }
}

double ef = total_energy(bodies);
print(`最終エネルギー保存率: ${(ef - e0) / math.abs(e0)}`);
```

---

## 学習ポイント

| 概念 | 内容 |
|------|------|
| `dim` 単位 | コンパイル時の物理単位チェック |
| `@layout_soa` | 構造体を配列に分離 → SIMD 連続アクセス |
| Leapfrog 積分 | エネルギーを保存する数値積分法 |
| `simd for` + `where` | i==j の自己相互作用をマスク処理 |
| `reduction(+:ke)` | 並列総和リダクション |

---

## 発展的なアイデア

- Barnes-Hut ツリーで O(N log N) に最適化
- GPU オフロード (`extern "FFI"` で CUDA カーネルを連携)
- 衝突検出を追加
