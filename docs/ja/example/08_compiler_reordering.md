# 例8: コンパイラ再配置最適化

> パイプラインストール、分岐予測、ループアンローリング、メモリバリアを
> dri コードで観察・最適化する方法を示す。

---

## 概要

現代のCPUは複雑なパイプラインマシンだ。  
このサンプルは **コード構造がハードウェア性能に与える影響** 4つを扱う。

| 最適化 | 問題 | 解決策 |
|--------|------|--------|
| パイプラインストール防止 | RAW ハザード | 4方向独立アキュムレータ |
| 分岐予測 | ランダムな真偽値 → 予測失敗率 ~50% | データをソート |
| ループアンロール + SIMD | ループオーバーヘッド | `simd for` + `step 4` |
| メモリバリア | 書き込み順序の再配置 | `sys.fence_full()` |

---

## 重要な構文

```dri
# 遅い: 毎イテレーションが前の結果を待つ → ストール
@bench
double sum_serial(double[] data) { ... }

# 速い: 4個の独立アキュムレータ → スーパースカラー並列実行
@bench
double sum_4accum(double[] data) {
    double s0 = 0.0; double s1 = 0.0;
    double s2 = 0.0; double s3 = 0.0;
    for (i in 0..n4 step 4) {
        s0 = s0 + data[i];     s1 = s1 + data[i+1];
        s2 = s2 + data[i+2];   s3 = s3 + data[i+3];
    }
    return s0 + s1 + s2 + s3;
}
```

```dri
# メモリバリア: データの可視性を保証
void safe_producer(mut Borrow<int[]> shared, int val) {
    shared[0] = val;
    sys.fence_full();  # この行以前の全書き込みが完了
    shared[1] = 1;     # ready フラグ
}
```

---

## ソース

完全なソース: [`example08.dri`](example08.dri)

---

## 構文リファレンス

| 構文 | 意味 |
|------|------|
| `@bench` | 関数の実行時間を stderr に出力 |
| `simd for` | OpenMP SIMD 自動ベクトル化ヒント |
| `for (i in 0..n step 4)` | `i += 4` インクリメント |
| `sys.fence_full()` | `std::atomic_thread_fence(seq_cst)` |
| `mut Borrow<T[]>` | `std::vector<T>&` — インプレース変更可能 |
