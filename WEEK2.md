# Week 2 — Data Structures & Memory (Days 8–14)

Theme: performance is memory. This week you build cache-aware containers and custom allocators — the toolkit that separates systems programmers from LeetCoders.

---

## Day 8 — `FlatMap`: sorted vector as a map

**Build:** a map built on a sorted `std::vector<std::pair<K,V>>`: `insert` (binary search + insert at position), `find` (`std::lower_bound`), `erase`, `operator[]`. Benchmark find() vs `std::map` and `std::unordered_map` at 100 / 10k / 1M elements.

**Why it exists:** `std::map` is a red-black tree — every node a separate allocation, pointer-chasing on every lookup, cache miss per level. A flat sorted vector is often faster below ~10k elements despite O(n) insertion, because contiguous memory wins. Trading systems use flat maps for price levels (your LOB!). `boost::flat_map` and C++23 `std::flat_map` exist because of this.

**How to write it perfectly:**
- Don't write your own binary search — `std::lower_bound` with a projection/comparator. Knowing the `<algorithm>` header IS knowing C++.
- Benchmark honestly: `-O2`, sanitizers off, warm-up pass, `std::chrono::steady_clock`, and use `benchmark::DoNotOptimize`-style tricks (`asm volatile("" :: "g"(x) : "memory")` or just accumulate results into a volatile).

**Reflexes:** container choice = access-pattern question, not API question ("how many elements? read-heavy? iteration?"); Big-O with a grain of salt — constants live in cache lines.

**Self-test:** write down your measured crossover point where `std::map` beats `FlatMap` (if ever).

**Read:** Chandler Carruth "Efficiency with Algorithms, Performance with Data Structures" (CppCon 2014) — the single best talk on this topic.

---

## Day 9 — Arena allocator (bump allocator)

**Build:** an `Arena`: one big `malloc`'d block; `allocate(size, align)` bumps a pointer (aligned with `std::align` or manual round-up); no per-object free — `reset()` frees everything at once. Then placement-new objects into it and measure vs `new`/`delete` for 1M small allocations.

**Why it exists:** general-purpose `malloc` does bookkeeping, locking, and fragmentation management you often don't need. Per-frame arenas (games), per-request arenas (servers), per-session arenas (trading) turn thousands of allocations into pointer bumps — 10–100× faster, and everything contiguous = cache-friendly. This is THE workhorse allocation pattern of high-performance systems.

**How to write it perfectly:**
- Alignment is the whole difficulty: `aligned = (current + align - 1) & ~(align - 1)`. Understand why misalignment is UB for some types.
- Placement new: `T* p = new (arena.allocate(sizeof(T), alignof(T))) T(args...);` — and who calls the destructor? (You do, manually, or you only put trivially-destructible types in — document the policy!)
- This is the Object Pool/Arena pattern: allocation strategy separated from object logic.

**Reflexes:** "what's the lifetime group?" — objects that die together should be allocated together; `alignof` awareness; placement new no longer scary.

**Self-test:** benchmark shows arena ≥10× faster than new/delete for small objects. ASan note: sanitizers can't see into your arena — understand what safety you gave up.

**Read:** learn what `std::pmr` (polymorphic memory resources) is — your Arena is basically `std::pmr::monotonic_buffer_resource`.

---

## Day 10 — Object pool with free list

**Build:** a fixed-capacity `Pool<T>`: pre-allocated slab of N slots; freed slots form an intrusive free list (store the next-free index/pointer IN the dead slot's memory via union or reinterpretation); `acquire()` / `release()` in O(1). Wrap acquisition in an RAII handle so release is automatic.

**Why it exists:** unlike the arena, a pool supports individual free — perfect for objects with independent lifetimes but identical size: orders in your LOB, connections, particles, messages. Every matching engine holds its orders in a pool. Reusing hot memory = staying in cache.

**How to write it perfectly:**
- The intrusive free list is the classic trick: dead objects' memory stores the bookkeeping. Zero overhead per live object.
- The RAII handle (a mini unique_ptr with a custom "deleter" pointing back to the pool) — Day 4 pays off.
- Type-safety at the boundary: the slab is raw bytes (`std::byte` buffer or `aligned_storage`-style); live slots are constructed via placement new, destroyed explicitly on release.

**Reflexes:** "fixed-size objects churning fast → pool" fires automatically; RAII handles for every acquire/release-shaped API (files, locks, pool slots — same pattern).

**Self-test:** stress: 1M random acquire/release ops, track live count, ASan-adjacent checks by poisoning freed slots with 0xDEAD pattern and asserting on double-release.

**Read:** Meyers Item 19 vs 18 (shared vs unique ownership cost); skim folly's `IndexedMemPool` header comments.

---

## Day 11 — Open-addressing hash map

**Build:** `HashMap<K,V>` with linear probing: single flat array of slots (state: empty/full/tombstone), power-of-two capacity, `insert`/`find`/`erase`, resize at 0.7 load factor. Benchmark find() vs `std::unordered_map` for 1M int keys.

**Why it exists:** `std::unordered_map` is chained — every node heap-allocated, pointer-chase per lookup. Open addressing keeps everything in one array: probes are sequential memory = prefetcher-friendly. This is how absl::flat_hash_map, folly F14, and every serious hash table works. Also: your ITCH order-ref→order lookup wants exactly this.

**How to write it perfectly:**
- Hash → index: `hash & (capacity-1)` (Day 6 reflex). Probe: `idx = (idx + 1) & mask`.
- Tombstones on erase (can't just mark empty — breaks probe chains). Understand WHY by drawing a probe sequence on paper first.
- Resize = rehash everything into a new array; tombstones die here.

**Reflexes:** counting allocations-per-operation when evaluating any container; "pointer chase" as a mental red flag; load factor as a tunable you actually think about.

**Self-test:** property test against `std::unordered_map` (1M random ops, states match); your find() should beat it by 2–5× on ints.

**Read:** Matt Kulukundis "Designing a Fast, Efficient, Cache-friendly Hash Table" (CppCon 2017 — the SwissTable talk).

---

## Day 12 — Zero-copy parsing with `string_view`

**Build:** a CSV parser that never allocates: `mmap`-or-read the file once into one buffer; `split(std::string_view, char) -> Vec<std::string_view>` returning views INTO the buffer; parse ints/doubles with `std::from_chars`; compute an aggregate over a 1M-row file. Benchmark vs a naive `std::getline` + `std::stoi` + `std::string` version.

**Why it exists:** parsing is where naive C++ bleeds: every substring a heap allocation, every stoi a locale-aware detour. Zero-copy (views into an immutable buffer) + `from_chars` (locale-free, non-throwing) is how fast parsers work — including your ITCH parser, JSON libraries (simdjson), and every feed handler.

**How to write it perfectly:**
- THE `string_view` rule: a view is a non-owning borrow — it must never outlive the buffer. Say out loud who owns the buffer and until when (Reflex #1 in a new costume).
- `std::from_chars`: no exceptions, no locales, returns errc — check it.
- Chunked reading if you skip mmap; that's fine, simplicity first.

**Reflexes:** returning `string_view` triggers an automatic lifetime audit; `std::stoi`/`atoi`/stringstream in a hot loop looks wrong now; "how many allocations does this parse do?" → answer should be ~0.

**Self-test:** naive vs zero-copy benchmark — expect 5–20×. Then intentionally create a dangling view (return a view to a local string) and watch ASan catch it. Feel the danger.

**Read:** cppreference `std::from_chars`; Core Guidelines on `string_view` parameters (F.15-ish region).

---

## Day 13 — Intrusive doubly-linked list + LRU cache

**Build:** an intrusive list (nodes contain `prev`/`next` embedded in the object; the list owns no memory) and use it + Day 11's HashMap to build an O(1) LRU cache: `get` moves node to front, `put` evicts from back.

**Why it exists:** `std::list` allocates per node; intrusive lists put the links inside objects you already allocated (e.g. in a pool) — zero allocation, and one object can sit in multiple lists at once (an order in a price-level list AND an LRU list). Kernels, allocators, and matching engines are wall-to-wall intrusive lists. LRU itself: the canonical interview design question, and how real caches work.

**How to write it perfectly:**
- The list is pure linkage: `link_front(Node&)`, `unlink(Node&)` — no ownership, document that loudly.
- Sentinel node (dummy head/tail) eliminates every null check — before/after with and without a sentinel is a lesson in itself.
- LRU: HashMap stores pointers to nodes; nodes carry key+value (needed to erase the map entry on eviction).

**Reflexes:** ownership vs linkage as separate concerns; sentinel/dummy nodes as a default trick; composing two O(1) structures for combined guarantees.

**Self-test:** LRU property test vs a naive vector implementation, 100k ops. All under ASan.

**Read:** Boost.Intrusive docs intro (just the rationale page).

---

## Day 14 — Consolidation + mini order book v0

**Build (75 min):** wire the week together into a toy order book: `Pool<Order>` (Day 10) owns orders; `HashMap<uint64_t, Order*>` (Day 11) for id→order; `FlatMap<Price, Level>` (Day 8) per side, each Level an intrusive list of orders (Day 13). Operations: add, cancel-by-id, best-bid/ask. No matching yet — this is the data layout your real LOB should converge to.

**Why:** you just built, from scratch, the actual memory architecture of a production matching engine. Every structure justified by an access pattern. This is also THE artifact to talk about at HFT interviews: "I built each container myself, here's why each one."

**Rebuild hour (45 min):** blank-file rebuilds: Arena (15 min), open-addressing insert/find (20 min), intrusive unlink (10 min).

**Reflexes check-in:** allocation-counting is automatic; container choice starts from access pattern; lifetime audits on every view/pointer return.

**Read:** update `MISTAKES.md`; skim your LOB repo asking "which of this week's structures should replace what I have?"
