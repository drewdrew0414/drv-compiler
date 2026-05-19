# Example 4: Generics + Trait Bounds + Polymorphism

> A data-structure library built on a sophisticated generic type system.
> Implements the same patterns as Rust's trait bounds, in dri.

---

## Overview

- `T implements Comparable + Hashable` trait bounds
- Generic sorted container, priority queue, and graph
- `Self` keyword for self-referential types
- Traits with default method implementations

---

## Code

```dri
module generic_collections;

# ── 1. Basic trait definitions ────────────────────────────────────

trait Comparable {
    int compare(Self other);      # returns -1, 0, or 1

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

# ── 2. Implement traits on concrete types ─────────────────────────

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
        return value * 2654435761;   # Knuth multiplicative hash
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

# ── 3. Generic sort (with trait bound) ───────────────────────────

# T must implement Comparable
void quicksort<T implements Comparable>(list<T> arr, int lo, int hi) {
    if (lo >= hi) { return; }

    T pivot = arr[(lo + hi) / 2];
    int i = lo;
    int j = hi;

    while (i <= j) {
        while (arr[i].less_than(pivot)) { i += 1; }
        while (pivot.less_than(arr[j])) { j -= 1; }
        if (i <= j) {
            # swap
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

# ── 4. Generic min/max ───────────────────────────────────────────

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

# ── 5. Generic binary search tree ────────────────────────────────

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

# ── 6. Priority queue (min-heap) ─────────────────────────────────

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

# ── Run tests ────────────────────────────────────────────────────

# sort test
list<IntWrapper> nums = [];
for (v of [5, 3, 8, 1, 9, 2, 7, 4, 6]) {
    IntWrapper w;
    w.value = v;
    lst.push(nums, w);
}
quicksort(nums, 0, lst.length(nums) - 1);
print("sorted:");
for (n of nums) { n.print_self(); }

# min/max
IntWrapper mn = find_min(nums);
IntWrapper mx = find_max(nums);
print(`min: ${mn.value}, max: ${mx.value}`);

# BST test
BST<IntWrapper> tree;
tree.size = 0;
for (v of [5, 3, 8, 1, 9]) {
    IntWrapper w; w.value = v;
    tree.insert(w);
}
IntWrapper search_val; search_val.value = 8;
print(`contains 8?: ${tree.contains(search_val)}`);

# heap sort
MinHeap<IntWrapper> heap;
for (v of [5, 3, 8, 1, 9, 2]) {
    IntWrapper w; w.value = v;
    heap.push(w);
}
print("heap extraction order:");
while (!heap.is_empty()) {
    IntWrapper out = heap.pop();
    print(out.value);
}
```

---

## Trait Bound Rules

```dri
# single bound
T find_min<T implements Comparable>(...)

# multiple bounds
T serialize<T implements Comparable + Printable + Hashable>(...)

# bound violation → compile error:
# "type 'Foo' does not implement trait 'Comparable'"
```

---

## Key Takeaways

- `Self` : refers to the implementing type inside a trait definition
- Default implementations (`less_than`, `greater_than`) : derived from a single `compare`
- Generic recursion (`BST_Node<T>`) combined with `Own<T>`
- Type-safe data structures enforced by trait bounds
