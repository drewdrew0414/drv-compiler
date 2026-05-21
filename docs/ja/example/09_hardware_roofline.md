# 例9: ハードウェア最大性能 — Roofline モデル

> STREAM ベンチマーク、キャッシュ階層帯域幅測定、Roofline モデルの
> テキスト可視化を dri コードで実装する。

---

## 概要

どんなアルゴリズムも2つのハードウェア限界のどちらかに当たる。

```
性能 [GFLOP/s] = min(Peak_FLOPS,  AI × Peak_BW)
                          ↑           ↑        ↑
                     計算限界      算術強度  メモリ限界
```

| 用語 | 定義 |
|------|------|
| 算術強度 (AI) | FLOP ÷ アクセスバイト数 |
| Memory-Bound | AI < Ridge Point → メモリ帯域がボトルネック |
| Compute-Bound | AI > Ridge Point → 演算スループットがボトルネック |
| Ridge Point | Peak_FLOPS ÷ Peak_BW [FLOP/Byte] |

---

## 重要な構文

```dri
# STREAM Triad — メモリ帯域幅の標準指標
@bench
void stream_triad(Borrow<double[]> a, Borrow<double[]> b,
                  mut Borrow<double[]> c, double scalar) {
    for (i in 0..lst.length(c)) {
        c[i] = a[i] + scalar * b[i];
    }
}

# 並列版
void stream_triad_parallel(Borrow<double[]> a, Borrow<double[]> b,
                            mut Borrow<double[]> c, double scalar) {
    parallel for (i in 0..lst.length(c)) {
        c[i] = a[i] + scalar * b[i];
    }
}
```

---

## ソース

完全なソース: [`example09.dri`](example09.dri)

---

## 構文リファレンス

| 構文 | 意味 |
|------|------|
| `Borrow<double[]>` | `const std::vector<double>&` |
| `mut Borrow<double[]>` | `std::vector<double>&` |
| `@bench` | 関数単位の実行時間測定 |
| `parallel for` | OpenMP 並列ループ |
| `sys.time_ms()` | 現在時刻 (ミリ秒、高精度) |
| `lst.fill(N, 0.0)` | サイズ N、初期値 0.0 のリスト作成 |
