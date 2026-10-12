# Emergency_triage_queue

Scenario: A casualty ward admits patients with a severity score. A waiting patient's severity can be raised at any time, a patient may leave before treatment, and patients who wait long must rise in priority so that no one waits forever.
What makes it hard
The indexed heap needs a locator hash table written from scratch, and the two structures must stay consistent through every swap, including deletion of an arbitrary element.
Aging looks as if it needs every key updated continuously. Linear aging needs no updates at all, while step-wise escalation needs scheduled events. Seeing and proving this is the design insight of the project.
The comparison of decrease-key against lazy re-insertion must match, or explain, published measurements.
Structure A: An indexed binary min-heap supporting insert, extract-min, decrease-key, delete(id) and build-heap, with a locator (open addressing, linear probing, with deletion) written from scratch mapping patient id to heap index.
Structure B: A binary heap with lazy invalidation: decrease-key and delete push a fresh entry and mark the old one stale through a version table; stale entries are discarded when they surface; the heap is compacted when stale entries outnumber live ones.
Application: A triage simulator that replays thirty simulated days of arrivals, severity revisions, departures and treatments against either structure, under two aging policies: linear, and step-wise escalation every 30 minutes of waiting. It reports waiting-time statistics per severity class.
Oracle: An unsorted array scanned linearly on every extract-min.
Adversarial targets: Sequences that maximise total sift work in A, sequences that maximise stale-entry build-up in B, and arrival patterns that would starve low-severity patients without aging.
Measurement: Time per operation and peak memory for A and B as the share of revisions rises from 0% to 90%, under both aging policies. Compare with the Analyst's model and with the published measurements; explain any gap.
Derivations
D1 (Structure A owner): BUILD-HEAP runs in O(n), proved by summing node heights, and decrease-key and delete run in O(log n) given O(1) expected locator lookup. Prove both.
D2 (Structure B owner): If the heap is compacted whenever stale entries exceed live ones, its size never exceeds twice the live count plus one, and every operation costs O(log n) amortised. Prove it with a potential function.
D3 (Architect): Under linear aging, priority s + c·(now − arrival), the relative order of any two waiting patients never changes, so the static key s − c·arrival works with no updates. Under step-wise escalation it does change, and the number of key updates equals the number of escalation steps taken. Prove both.
D4 (Oracle owner): The locator invariant pos[h[i].id] = i holds after every sift and after the swap-with-last in delete, and heap order plus locator consistency is necessary and sufficient for a valid indexed heap. Prove both.
D5 (Adversary): Some input forces Ω(n log n) total comparisons over n extract-min operations. Prove it from the comparison-sorting lower bound and the O(n) build-heap bound, and give a generator that approaches it.
D6 (Analyst): Closed-form predicted costs for A and B as functions of n, the stale fraction and the aging policy, written before measuring.
Reference papers and study material
J. W. J. Williams, “Algorithm 232: Heapsort,” Communications of the ACM 7(6):347–348, 1964.
R. W. Floyd, “Algorithm 245: Treesort 3,” Communications of the ACM 7(12):701, 1964.
M. Chen, R. A. Chowdhury, V. Ramachandran, D. L. Roche and L. Tong, “Priority queues and Dijkstra's algorithm,” Technical Report TR-07-54, Department of Computer Sciences, University of Texas at Austin, 2007 (decrease-key against re-insertion, measured).
T. H. Cormen, C. E. Leiserson, R. L. Rivest and C. Stein, Introduction to Algorithms, 3rd ed., MIT Press, 2009: Chapter 6 (§6.5 priority queues and handles) and Chapter 11 (§11.4 open addressing).
R. Sedgewick and K. Wayne, Algorithms, 4th ed., Addison-Wesley, 2011: §2.4 (indexed priority queue).
D. E. Knuth, The Art of Computer Programming, Vol. 3, 2nd ed., Addison-Wesley, 1998, §6.4 (hashing and open addressing).
