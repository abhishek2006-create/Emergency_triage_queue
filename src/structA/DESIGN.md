# Structure A design notes

Structure A is an indexed binary min-heap. The heap keeps the next patient
to treat at the root, and a locator table maps each waiting patient's ID
to their current index in the heap. The shared `INTERFACE.md` will define
the public operations, key type, and tie-breaking rule.

## Heap and locator

- Store the heap in an array. Whenever two entries swap, update both
  patients' indices in the locator. The invariant is
  `pos[heap[i].id] == i` for every occupied index.
- Build the locator from scratch using open addressing and linear
  probing. A lookup continues past deleted slots and stops at a slot
  that has never been used. Rehash as needed to keep probing efficient.
- On insertion, append the new entry, record its index, and sift it up.
  On decrease-key, find the patient using the locator, change the key,
  and sift the entry up.
- To delete a patient, swap their entry with the last entry and update
  the locator. Remove the last slot and erase the deleted ID from the
  locator. If a replacement moved into the gap, sift it up if it
  precedes its parent. Otherwise, sift it down. Extract-min uses the
  same procedure on the root.
- To build a heap from an array, fill the locator with the initial
  indices, then sift down from the last internal node to the root.
  Each swap updates the locator during this pass.

## Hand checks

| Case | Expected result |
| --- | --- |
| Insert IDs 1, 2, and 3 with keys 20, 15, and 5 | ID 3 is at the root, and each locator entry points to its own patient. |
| Decrease ID 1's key from 20 to 0 | ID 1 moves to the root, and its locator index becomes 0. |
| Delete a non-root patient, then revise the patient that moved into the gap | The revision finds the moved patient using its updated index. |
| Insert two IDs that hash to the same slot, then delete the first | Lookup still finds the second ID by probing past the deleted slot. |