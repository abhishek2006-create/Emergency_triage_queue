# D1: Indexed Heap Complexity Proof

Author: Structure A owner (enter team member name before submission)

## Definitions and assumptions

Let n be the number of live patients. Structure A stores n entries in an array-backed binary min-heap H, and a custom open-addressed locator maps each patient ID to its current heap index. The priority order is lexicographic in (priority key, arrival time, patient ID). A heap of n entries has height at most floor(log2 n). Hash-table probes take expected O(1) time under the assumed hash distribution and controlled load factor; rehashes are analyzed amortized.

## Proof 1: Bottom-up BUILD-HEAP is O(n)

Floyd's method calls siftDown on the internal nodes, working from index floor(n/2)-1 toward the root. A node of height h can move down at most h levels. In a complete binary tree, at most ceil(n / 2^(h+1)) + 1 nodes have height h; the loose additive count affects only O(log n) heights. Thus total sift work is bounded by O(n * sum from h=0 to infinity of h/2^h + (log n)^2) = O(n), because sum h/2^h = 2 and (log n)^2 = O(n). A tighter standard bound uses at most ceil(n/2^(h+1)) nodes at each height and gives O(n) directly. The lower bound is Omega(n) because all n entries must be read; therefore heap construction is Theta(n).

## Locator construction within BUILD-HEAP

The implementation first inserts the n distinct IDs into an initially empty locator. A bounded load factor with geometric rehashing gives O(n) expected total initialization time: each expansion copies O(capacity) entries, and capacities form a geometric series. Then the bottom-up heap pass costs O(n), including O(1) expected locator index updates per swap. Therefore the entire buildHeap implementation is O(n) expected amortized work. It builds a temporary heap first, so rejection of a duplicate ID does not replace the original queue.

## Proof 2: decreaseKey is O(log n) expected amortized

The locator identifies a live patient in expected O(1) time. A strictly smaller priority key cannot violate the heap property with a child; only the parent relation may fail. siftUp traverses at most the height of the heap, floor(log2 n). Every swap takes O(1) array work and two expected O(1) locator updates. Hence total expected amortized time is O(1 + log n) = O(log n). Missing IDs and non-decreasing keys return an error without changing the logical queue.

## Proof 3: delete(id) is O(log n) expected amortized

The locator identifies index i in expected O(1). Swap H[i] with H[n-1], erase the removed ID, and pop the last entry. The replacement at i can violate heap order either upward or downward. If smaller than its parent, run siftUp; otherwise run siftDown. At most one root-to-leaf or leaf-to-root path is traversed, with at most O(log n) swaps and locator updates. Thus delete(id) takes O(log n) expected amortized time. extractMin uses the same removal mechanism at index 0.

## Scope of the bounds

A hostile sequence of hash collisions can require O(n) probes; an individual locator insertion that triggers resizing can also cost O(n). Accordingly O(log n) per targeted heap operation is an expected amortized claim under the hashing and resizing assumptions, not a worst-case guarantee for each call. The binary-heap comparisons and swaps alone have worst-case O(log n) bounds.
