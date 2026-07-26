# Week 1 — Ownership & Value Semantics (Days 1–7)

Theme: you can't have instincts until ownership is automatic. This week you rebuild the core of the standard library yourself. It will feel humbling; that's the point. After this week, `std::vector` is no longer magic — it's something you've built.

---

## Day 1 — `Vec<int>`: a dynamic array from scratch

**Build:** a growable array of `int` (not a template yet): `push_back`, `pop_back`, `operator[]`, `size`, `capacity`, growth-by-doubling. Manual `new[]`/`delete[]` — the ONLY days you're allowed naked new, because the whole point is feeling the pain RAII removes.

**Why it exists:** `std::vector` is the most used data structure in C++; contiguous memory = cache-friendly = the default container for everything, including every trading system's hot path.

**How to write it perfectly:**
- Constructor establishes the invariant (`size_ <= capacity_`, `data_` valid or null). Destructor releases. That pairing is RAII.
- Growth: allocate new block, copy elements, delete old, THEN update members — think about what happens if the copy throws halfway.
- Write the destructor FIRST, right after the constructor. Always.

**Reflexes:** every `new[]` immediately raises "where is the delete[]?"; invariants written as comments + asserts at the top of the class.

**Self-test:** run with `-fsanitize=address`, push 1M elements, no leaks. Then add a `Vec v2 = v1;` — watch it double-free and crash. Understand exactly why. That crash is tomorrow's lesson.

**Read (15 min):** learncpp.com chapters on destructors and dynamic memory; Meyers Item 17 (special member functions).

---

## Day 2 — Rule of Five: make `Vec` copyable and movable

**Build:** copy constructor, copy assignment, move constructor, move assignment, destructor for Day 1's `Vec`. Then the copy-and-swap idiom as an alternative assignment. Add prints in each to SEE which gets called when.

**Why it exists:** this is the heart of C++'s value semantics — the thing Go/Python/Java don't have. Moves are why C++ can return big objects by value at zero cost.

**How to write it perfectly:**
- Move ctor: steal pointers, null the source (`other.data_ = nullptr; other.size_ = 0`), mark `noexcept` — vector reallocation only uses your move if it's `noexcept` (look up why: strong exception guarantee).
- Self-assignment check, or design it away with copy-and-swap.
- Experiment: `Vec b = std::move(a); a.push_back(1);` — moved-from must be valid-but-unspecified; yours should be empty-and-usable.

**Reflexes:** typing a destructor auto-triggers "now copy/move or `=delete` them"; `noexcept` on every move; `std::move` is just a cast — it moves nothing by itself.

**Self-test:** predict the copy/move print sequence for `Vec f() { Vec v; ...; return v; }` before running (learn NRVO — you'll likely see NO copy or move).

**Read:** Meyers Items 23–25 (std::move, universal refs, noexcept moves).

---

## Day 3 — Templates + iterators: `Vec<T>` that works with the STL

**Build:** convert `Vec` to `template<typename T>`, add `begin()`/`end()` (raw pointers are valid iterators!), make range-for, `std::sort`, and `std::find` work on it. Add `emplace_back` with perfect forwarding (`template<class... Args> T& emplace_back(Args&&... args)`).

**Why it exists:** the STL's genius is that algorithms and containers are decoupled through iterators. Your types become first-class citizens by exposing them.

**How to write it perfectly:**
- Templates live entirely in headers — understand why (instantiation needs the definition).
- Copying elements on growth: use `std::move_if_noexcept` and understand the choice.
- Placement new for `emplace_back` is a preview of Day 9; using `push_back(T(args...))` today is acceptable.

**Reflexes:** "will this work with a range-for?" as a design question; reading template compiler errors bottom-up looking for "required from here."

**Self-test:** `Vec<std::string>` with 100k strings under ASan; `std::sort` it.

**Read:** learncpp templates chapters; cppreference on iterator requirements (skim).

---

## Day 4 — `UniquePtr<T>`: ownership as a type

**Build:** your own `unique_ptr`: ctor from raw pointer, destructor, move-only (copy `= delete`), `operator*`, `operator->`, `get`, `release`, `reset`. Then a `make_unique` clone. Then use it: a small `Node`-based linked list where nodes are held by `UniquePtr` — no delete anywhere.

**Why it exists:** unique ownership as a compile-time guarantee. This single type eliminates the majority of C++ memory bugs. It's also zero-cost — same assembly as a raw pointer (verify on godbolt).

**How to write it perfectly:**
- `= delete` the copy operations explicitly — the compiler now enforces your ownership design (Reflex #9: make it a compile error).
- `release()` returns the pointer AND nulls the member — transferring ownership out.
- Think through: what should `reset(p)` do if `p == get()`?

**Reflexes:** naked `new` in any codebase now looks like a bug; function signatures communicate ownership (`unique_ptr` param = "I take ownership", raw pointer = "I only borrow").

**Self-test:** godbolt: compare `-O2` assembly of your UniquePtr deref vs raw pointer — identical. Zero-cost abstraction, seen with your own eyes.

**Read:** Meyers Items 18–21 (smart pointers); Core Guidelines R.20–R.37 (skim).

---

## Day 5 — `String`: small buffer optimization (SBO)

**Build:** a string class storing ≤15 chars inline (no heap) and longer strings on the heap — a union/buffer + a flag (or clever size encoding). Full Rule of Five (this is the real exam on Days 1–2). `c_str()`, `size()`, `operator+=`, `operator==`.

**Why it exists:** real `std::string` does exactly this; most strings are short, and SBO eliminates the allocation, which matters enormously in hot paths. This is your first taste of "the fast path is a different code path."

**How to write it perfectly:**
- Every Rule-of-Five member now has TWO cases (inline vs heap) on each side. Draw the 4-quadrant table (src inline/heap × copy/move) before writing code.
- Moves from inline storage COPY the bytes (can't steal a buffer that lives inside the object) — a genuinely instructive surprise.

**Reflexes:** "how big is this object, where do its bytes live?" (`sizeof` everything); allocation in the hot path is guilty until proven necessary.

**Self-test:** ASan clean under a stress loop of random copies/moves/concats of mixed sizes. Log heap allocations (count them in a static) — short strings must allocate zero.

**Read:** Raymond Chen or folly `FBString` design notes on SBO (search "small string optimization explained").

---

## Day 6 — `FixedRing<T, N>`: bounded ring buffer, single-threaded

**Build:** compile-time-sized circular queue on a `std::array`-like buffer: `push`, `pop`, `front`, `full`, `empty`, head/tail indices with power-of-two wraparound (`idx & (N-1)`), `static_assert` that N is a power of two.

**Why it exists:** the bounded queue is THE hot data structure in trading/telecom/audio. You built the threaded Go version already — today is the single-threaded C++ core, done perfectly, as the foundation for Week 3's SPSC.

**How to write it perfectly:**
- No modulo: `& (N-1)` — check godbolt that `% N` with non-power-of-2 emits `div` (~20+ cycles) vs `and` (1 cycle).
- Decide and document the empty-vs-full convention (keep-one-slot-free, or separate count, or monotonic indices — pick monotonic indices `head_++`, wrap on access: it's what your Go version does and what Week 3 needs).
- `static_assert((N & (N-1)) == 0)` — Reflex #9 again.

**Reflexes:** `static_assert` reaching for compile-time checks; power-of-two sizes as an automatic default; suspicion of `%` and `/` in hot loops.

**Self-test:** property test: mirror 1M random push/pops against a `std::deque`, states must match at every step.

**Read:** your own Go ring buffer + its README, translating each design decision into C++ terms.

---

## Day 7 — Consolidation + `Matrix` mini-capstone

**Build (60 min):** a `Matrix` class: single flat `Vec<double>` (yours!) with `operator()(row, col)` doing `row * cols + col`. Multiply two matrices naive (i,j,k), then loop-reordered (i,k,j), benchmark both at 512×512.

**Why:** ties the week together AND delivers the single most visceral cache lesson in programming: identical math, 5–10× speed difference, purely from memory access order.

**Rebuild hour (60 min):** from BLANK files, no peeking, 20 min each: `Vec<T>` core, `UniquePtr`, `FixedRing`. Whatever you can't rebuild, that's your weekend review.

**Reflexes check-in — by tonight these fire automatically:** destructor→Rule of Five; new→owner; move→noexcept; hot loop→memory layout.

**Read:** update `MISTAKES.md`; watch Mike Acton "Data-Oriented Design" (CppCon 2014) — the ideological foundation for Week 2.
