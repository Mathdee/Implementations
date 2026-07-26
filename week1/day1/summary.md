# Day 1 — Vec: growable array of int, manual new[]/delete[]

Built a `Vec` class from scratch, no `std::vector`, no templates yet, just
`int`. Point of the exercise was to feel the pain RAII normally hides —
this is the one day naked `new`/`delete` is actually allowed.

Wrote constructor and destructor first, before any real logic. Constructor
just sets `data_ = nullptr, size_ = 0, capacity_ = 0` — no allocation needed
yet, since an empty Vec has nothing to store. Destructor is one line,
`delete[] data_;`, safe even on `nullptr` since that's a no-op by the
language rules.

Invariant written as a comment at the top of the class: `size_ <= capacity_`,
and `data_` is `nullptr` iff `capacity_ == 0`. `operator[]` asserts
`index < size_` before indexing — turns silent memory corruption into an
immediate crash with a clear message in debug builds.

Growth (`grow_`) is the core of the day. Order matters a lot here:

1. allocate new block
2. copy old elements into it
3. delete[] old block
4. only then reassign `data_`/`capacity_`

Reasoning it through: if you reassign `data_` before deleting the old block,
`delete[] data_` ends up deleting the NEW block instead, leaking the old one
and destroying the new one you just built. Doing delete last means nothing
observable changes until the risky part (the copy) has fully succeeded.

`push_back` doubles capacity when full (starting at 1, not 0, since doubling
zero stays zero forever). Doubling instead of +1 each time is what makes
push_back O(1) amortized instead of O(n) per call.

## Self-test results

- Pushed 1M elements under `-fsanitize=address` — clean, no leaks, capacity
  landed on 1,048,576 (2^20), exactly the first power of 2 >= 1,000,000,
  confirming the doubling logic is correct.
- Then wrote `Vec v2 = v1;` on purpose, no copy constructor written yet.
  Got an immediate ASan "attempting double-free" crash.

## Why it crashed

Never wrote a copy constructor, so the compiler generated a default one
that does a member-wise copy — including `data_`, which is just a pointer.
Copying a pointer copies the ADDRESS, not the array. `v1` and `v2` end up
pointing at the exact same heap block, both thinking they own it. First
destructor to run frees it legitimately; second one frees the same address
again, which is the double-free. This is the shallow-copy problem, and
exactly why the Rule of Three/Five exists — if you write a destructor, you
almost certainly need a real copy constructor too. That's Day 2.