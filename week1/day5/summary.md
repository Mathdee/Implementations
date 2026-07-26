# Day 5 — String with Small Buffer Optimization (SBO)

Built a `Mystring` class that stores short strings (<=15 chars) directly inside
the object, on the stack, and only goes to the heap once a string gets longer
than that. This is basically what real `std::string` implementations do.

Core trick is a `union`:

```text
union {
    char inline_data_[16];
    struct { char* data_; size_t capacity_; } heap_;
};
```

A union lets two incompatible representations share the same bytes instead of
storing both side by side. Only one is ever "active" at a time, so `sizeof`
stays small (~24 bytes) instead of ~40 if I'd kept a pointer AND a buffer as
separate members. The tradeoff: nothing tracks which one is active for you,
so I need my own `is_heap_` flag and have to trust it everywhere.

Because of that flag, every Rule-of-Five function (ctor, dtor, copy ctor,
copy assignment, move ctor, move assignment) now has a real branch in it —
inline case vs heap case — instead of always doing the same thing like in
`Vec`. Wrote out a table first (source inline/heap x copy/move) before
touching code, which helped, since one cell (inline source with size > 15)
is actually unreachable by the class's own invariant.

Genuine surprise: moving FROM inline storage still means copying the bytes.
There's no pointer to steal — the data lives inside the object itself. Only
the heap case gets a real "steal the pointer" move.

Bug I hit and fixed: in `operator+=`, the inline-to-heap transition case
allocated a new buffer, filled it correctly, and then never actually saved
it into `heap_.data_`/`capacity_`/`is_heap_`. Compiled fine, ran fine on
short inputs, but leaked memory the moment a string crossed 15 chars,
because the object still thought it was inline and the destructor never
freed the orphaned block. Classic case of "build the new state fully before
touching the old state" (same rule from Day 1's `grow_`) — except here I
also forgot the "commit" step at the end, not just the ordering.

Also added a static `heap_alloc_count` and bumped it at every `new char[]`
call site (5 total: ctor, copy ctor, copy assignment, and both `+=` growth
paths). Move ctor/assignment never touch it — correctly, since moving out
of a heap string just steals the pointer, no allocation needed.

## Self-test results

- 100k iterations of construct/copy/move/append/assign, all staying short
  → **0 heap allocations**. This is the actual proof SBO is doing its job,
  not just "looks right in the code."
- 50k iterations mixing short and long strings, forcing heap growth and
  reallocation → ran clean under ASan, no leaks, no double-frees. This
  is what confirmed the `operator+=` bug above was actually fixed, since
  before the fix this exact test would've leaked hard.

## Takeaway

Allocation-counting turned "I think this is zero-alloc" into something I
could actually check. Same idea as writing invariants as asserts on Day 1 —
don't trust that the code does what you meant, make it prove it.