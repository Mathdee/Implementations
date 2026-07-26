# Day 2 — Rule of Five: make Vec copyable and movable

Direct follow-up to yesterday's double-free crash. Once you write a
destructor, the compiler stops safely auto-generating some of the others,
so you need to explicitly write all five: destructor (already had it), copy
constructor, copy assignment, move constructor, move assignment.

## Copy constructor

`Vec(const Vec& other)` — allocate a new block sized to `other.capacity_`,
copy `other`'s elements into it, set `this`'s members from `other`. First
attempt had a real bug: wrote `this->data_ = data_;` instead of
`other.data_`, which compiled fine but silently copied garbage, since a
bare unqualified name inside a member function ALWAYS refers to `this`,
never to the other parameter, even in a copy constructor. Good lesson —
that mistake gives no compiler error, just wrong behavior.

## Copy assignment

`Vec& operator=(const Vec& other)` — different from the copy constructor
because `this` already owns memory that needs handling. Two failure modes
to think through first:

- self-assignment (`v1 = v1;`): if you delete `this->data_` before copying,
  you've also deleted `other.data_` since they're the same object, so the
  copy loop reads freed memory.
- forgetting to free the OLD block before overwriting `data_`: straight
  memory leak.

Fix: allocate + copy new block first (safe, doesn't touch old state),
THEN delete old block, THEN reassign. Guard `if (this == &other) return *this;`
at the top handles self-assignment directly.

## Move constructor

`Vec(Vec&& other) noexcept` — steal `data_`/`size_`/`capacity_` from
`other`, then null out `other`'s members so its destructor later does
nothing. `&&` (rvalue reference) is the compiler's way of saying "this
argument is disposable, safe to steal from." `std::move` itself is just a
cast to trigger this overload — it doesn't move anything by itself.

`noexcept` matters here specifically because containers like
`std::vector` need to guarantee they can roll back if something throws
mid-reallocation. Without `noexcept`, they silently fall back to the
slower copy constructor instead of trusting the move, even when a move
was technically possible.

## Move assignment + self-move problem

Same self-assignment danger as copy assignment, but sneakier: with
`a = std::move(a);`, deleting `this->data_` first also deletes `other.data_`
since it's the same object — traced through by hand and found the actual
failure mode isn't a crash, it's silent data loss (ends up with an empty,
valid-but-wrong object). Guard with `this == &other` fixes it.

## Copy-and-swap idiom

Spec's suggested alternative: unify copy assignment and move assignment
into one function, `Vec& operator=(Vec other)`, taking the parameter BY
VALUE instead of by reference. Taking by value forces the compiler to
copy-construct (or move-construct, depending on the caller) a fresh, truly
separate object before the function body even runs — so `this == &other`
literally can't ever be true, no explicit guard needed. Then just
`swap(*this, other); return *this;` — old data gets destroyed automatically
when `other` (now holding the old contents) goes out of scope.

Wrote a `friend void swap(Vec&, Vec&) noexcept` that swaps the three
members via `std::swap`, using `using std::swap;` first to enable ADL
(lets a more specific swap get picked if one exists for the type, falls
back to std::swap otherwise).

## Self-test: NRVO

```cpp
Vec f() {
    Vec v;
    v.push_back(1);
    return v;
}
Vec result = f();
```

Predicted zero copy/move prints because of NRVO — the compiler builds `v`
directly inside `result`'s memory, no separate object ever exists to copy
or move from. Confirmed correct. Worth knowing this is a compiler
OPTIMIZATION, not a language guarantee pre-C++17 — compiling with `-O0`
vs `-O2` can actually change whether it kicks in, which is why the move
constructor still needs to exist and be correct as a fallback.