# 例 4: ジェネリック + トレイト境界 + ポリモーフィズム

> 複雑なジェネリック型システムを活用したデータ構造ライブラリ。
> Rust のトレイト境界と同様のパターンを dri で実装する。

---

## 概要

- `T implements Comparable + Hashable` トレイト境界
- ジェネリックなソート済みコンテナ、優先度キュー、グラフ
- `Self` キーワードによる自己参照型
- デフォルト実装を持つトレイト

---

## コード

```dri
module generic_collections;

# ── 1. 基本的なトレイト定義 ───────────────────────────────────

trait Comparable {
    int compare(Self other);      # -1、0、1 を返す

    boolean less_than(Self other) {
        return compare(other) < 0;
    }
    boolean greater_than(Self other) {
        return compare(other) > 0;
    }
    boolean equals(Self other) {
        return compare(other) == 0;
    }
}

trait Printable {
    String to_string();

    void print_self() {
        print(to_string());
    }
}

trait Hashable {
    int hash();
}

# ── 2. 具体的な型へのトレイト実装 ────────────────────────────

class IntWrapper {
    int value;
}

impl Comparable for IntWrapper {
    int compare(Self other) {
        if (value < other.value) { return -1; }
        if (value > other.value) { return 1; }
        return 0;
    }
}

impl Printable for IntWrapper {
    String to_string() {
        return str.from_int(value);
    }
}

impl Hashable for IntWrapper {
    int hash() {
        return value * 2654435761;   # Knuth 乗算ハッシュ
    }
}

class StringWrapper {
    String value;
}

impl Comparable for StringWrapper {
    int compare(Self other) {
        if (value < other.value) { return -1; }
        if (value > other.value) { return 1; }
        return 0;
    }
}

impl Printable for StringWrapper {
    String to_string() {
        return `"${value}"`;
    }
}

# ── 3. ジェネリックなソート (トレイト境界付き) ───────────────

# T が Comparable を実装している必要がある
void quicksort<T implements Comparable>(list<T> arr, int lo, int hi) {
    if (lo >= hi) { return; }

    T pivot = arr[(lo + hi) / 2];
    int i = lo;
    int j = hi;

    while (i <= j) {
        while (arr[i].less_than(pivot)) { i += 1; }
        while (pivot.less_than(arr[j])) { j -= 1; }
        if (i <= j) {
            # スワップ
            T tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
            i += 1;
            j -= 1;
        }
    }
    quicksort(arr, lo, j);
    quicksort(arr, i, hi);
}

# ── 4. ジェネリックな最小値/最大値 ──────────────────────────

T find_min<T implements Comparable>(list<T> arr) {
    T result = arr[0];
    for (i in 1..lst.length(arr)) {
        if (arr[i].less_than(result)) {
            result = arr[i];
        }
    }
    return result;
}

T find_max<T implements Comparable>(list<T> arr) {
    T result = arr[0];
    for (i in 1..lst.length(arr)) {
        if (arr[i].greater_than(result)) {
            result = arr[i];
        }
    }
    return result;
}

# ── 5. ジェネリックな二分探索木 ──────────────────────────────

class BST_Node<T implements Comparable> {
    T value;
    Own<BST_Node<T>> left;
    Own<BST_Node<T>> right;
}

class BST<T implements Comparable> {
    Own<BST_Node<T>> root;
    int size;

    void insert(T val) {
        root = insert_node(root, val);
        size += 1;
    }

    Own<BST_Node<T>> insert_node(Own<BST_Node<T>> node, T val) {
        if (node == null) {
            Own<BST_Node<T>> n = new BST_Node<T>();
            n.value = val;
            return move n;
        }
        if (val.less_than(node.value)) {
            node.left = insert_node(move node.left, val);
        } else if (val.greater_than(node.value)) {
            node.right = insert_node(move node.right, val);
        }
        return move node;
    }

    boolean contains(T val) {
        return search_node(root, val);
    }

    boolean search_node(Borrow<BST_Node<T>> node, T val) {
        if (node == null) { return false; }
        if (val.equals(node.value)) { return true; }
        if (val.less_than(node.value)) {
            return search_node(node.left, val);
        }
        return search_node(node.right, val);
    }
}

# ── 6. 優先度キュー (最小ヒープ) ─────────────────────────────

class MinHeap<T implements Comparable> {
    list<T> data;

    void push(T val) {
        lst.push(data, val);
        int i = lst.length(data) - 1;
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (data[i].less_than(data[parent])) {
                T tmp = data[i];
                data[i] = data[parent];
                data[parent] = tmp;
                i = parent;
            } else {
                break;
            }
        }
    }

    T pop() {
        int n = lst.length(data);
        T result = data[0];
        data[0] = lst.pop(data);
        int i = 0;
        while (true) {
            int l = 2 * i + 1;
            int r = 2 * i + 2;
            int smallest = i;
            if (l < n and data[l].less_than(data[smallest])) { smallest = l; }
            if (r < n and data[r].less_than(data[smallest])) { smallest = r; }
            if (smallest == i) { break; }
            T tmp = data[i];
            data[i] = data[smallest];
            data[smallest] = tmp;
            i = smallest;
        }
        return result;
    }

    boolean is_empty() {
        return lst.is_empty(data);
    }
}

# ── テスト実行 ───────────────────────────────────────────────

# ソートテスト
list<IntWrapper> nums = [];
for (v of [5, 3, 8, 1, 9, 2, 7, 4, 6]) {
    IntWrapper w;
    w.value = v;
    lst.push(nums, w);
}
quicksort(nums, 0, lst.length(nums) - 1);
print("ソート結果:");
for (n of nums) { n.print_self(); }

# 最小値/最大値
IntWrapper mn = find_min(nums);
IntWrapper mx = find_max(nums);
print(`最小値: ${mn.value}, 最大値: ${mx.value}`);

# BST テスト
BST<IntWrapper> tree;
tree.size = 0;
for (v of [5, 3, 8, 1, 9]) {
    IntWrapper w; w.value = v;
    tree.insert(w);
}
IntWrapper search_val; search_val.value = 8;
print(`8 を含む?: ${tree.contains(search_val)}`);

# ヒープソート
MinHeap<IntWrapper> heap;
for (v of [5, 3, 8, 1, 9, 2]) {
    IntWrapper w; w.value = v;
    heap.push(w);
}
print("ヒープからの取り出し順:");
while (!heap.is_empty()) {
    IntWrapper out = heap.pop();
    print(out.value);
}
```

---

## トレイト境界のルール

```dri
# 単一境界
T find_min<T implements Comparable>(...)

# 複合境界
T serialize<T implements Comparable + Printable + Hashable>(...)

# 境界違反 → コンパイルエラー:
# "type 'Foo' does not implement trait 'Comparable'"
```

---

## 学習ポイント

- `Self` : トレイト内で実装型自身を参照するキーワード
- デフォルト実装 (`less_than`, `greater_than`) : 1 つの `compare` から自動提供
- ジェネリックな再帰 (`BST_Node<T>`) と `Own<T>` の組み合わせ
- トレイト境界による型安全なデータ構造の実装
