# D4: Indexed Heap Locator Invariant and Correctness

Author: Abhimanyu Kumar (250002005)

## Definitions

Let H[0..n-1] be Structure A's binary min-heap. Let pos map every live patient ID to the index where that patient appears. IDs are unique. Define the locator invariant:

    for every i in [0,n), pos[H[i].patient_id] = i.

We additionally require pos to contain exactly n mappings (one for every live patient), and each heap entry has a distinct ID.

## Preservation under a swap

Suppose the invariant holds before swapping H[i] and H[j]. Let the respective IDs be a and b. The implementation first exchanges H[i] and H[j], then sets pos[b] = i and pos[a] = j. All other H[k] and pos mappings are unchanged. Therefore the invariant holds for i, j, and every other k. This is true for every swap used by sift-up and sift-down. By induction over the number of swaps, both procedures preserve the locator invariant.

## Preservation under delete(id)

The locator gives index i for the requested ID. If i is the last index n-1, the algorithm erases its mapping and removes that last element. Otherwise it swaps H[i] with H[n-1], updating both mappings. The removed ID is now at n-1. The algorithm erases that ID's mapping and pops the last element. For the retained replacement entry at i, its locator already points to i. The remaining heap elements are unchanged. Subsequent sift-up or sift-down swaps preserve the invariant by the preceding argument. Therefore deletion preserves locator consistency.

## Necessity and sufficiency

A valid indexed min-heap must simultaneously satisfy (1) heap order under the specified lexicographic comparator and (2) a one-to-one, exact mapping between all live patient IDs and their current heap positions. These properties are necessary: violating heap order makes the root unreliable, and violating the locator prevents correct targeted updates and deletions. Conversely, suppose the heap contains unique IDs, every parent precedes or ties with its children, the locator maps exactly those IDs to their true positions, and no extra locator mappings exist. Every descendant is no smaller than its ancestors by transitivity, so H[0] is minimum. Locator lookup identifies the right element for targeted operations, and swaps and removals preserve the mapping. Thus the representation is a valid indexed binary min-heap. Heap order and locator consistency alone are insufficient if duplicate IDs or extra locator mappings are allowed, so the exact-bijection conditions are part of the invariant.

## Complexity

A check of every heap parent-child pair costs O(n). A check of the locator relation for all n heap entries costs O(n) expected time with expected O(1) hash lookup. Exact mapping size and unique-ID checks rule out orphaned mappings and duplicated IDs. Individual sifts take O(log n) swaps; deletion costs O(log n) expected time after locator lookup. These complexity claims assume bounded hash load factor and well-distributed hashes.
