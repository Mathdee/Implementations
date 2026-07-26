# Week 3 — Concurrency (Days 15–21)

Theme: from mutexes to lock-free, in the right order. Rule for the week: **correct first, fast second** — a fast wrong queue is worthless. New tool: `-fsanitize=thread` (TSan) — run it instead of ASan on every threaded build (they can't run together).

---

## Day 15 — Thread-safe queue + producer/consumer

**Build:** `BlockingQueue<T>`: `std::mutex` + `std::condition_variable`, `push`, blocking `pop`, bounded capacity (producers block when full), plus a `close()` that wakes everyone and makes pop return nullopt. Demo: 4 producers, 4 consumers, 1M items, exact-count verification.

**Why it exists:** the mutex+condvar queue is the bread-and-butter of all multithreaded systems — thread pools, pipelines, logging. You must be fluent in it before touching atomics, and 95% of the time it's also the RIGHT choice (lock-free is a specialist tool).

**How to write it perfectly:**
- Condvar wait ALWAYS with a predicate: `cv.wait(lock, [&]{ return !q.empty() || closed; })` — this handles spurious wakeups automatically. Naked `cv.wait(lock)` is a bug reflex.
- `std::unique_lock` for waiting, `std::lock_guard`/`std::scoped_lock` everywhere else — know why (wait needs unlock/relock).
- Notify outside the lock where possible; `notify_one` vs `notify_all` — reason about which and when (close() needs all).

**Reflexes:** shared data → "which mutex protects this?" written as a comment next to the member; every condvar has a predicate; lock scope as small as possible, never across a callback you don't control.

**Self-test:** TSan-clean at 8 threads / 1M items. Then remove the predicate and hunt the hang — feel spurious wakeup.

**Read:** "C++ Concurrency in Action" (Williams) ch. 4 — or learncpp/cppreference condition_variable if you don't have the book.

---

## Day 16 — Thread pool

**Build:** a fixed-size `ThreadPool` on top of Day 15's queue: worker threads loop-popping tasks (`std::function<void()>` — accepted cost for now), `submit()` returning `std::future` via `std::packaged_task`, clean shutdown in the destructor (close queue, join all). Demo: parallel sum of a 100M array, measure scaling 1→8 threads.

**Why it exists:** thread creation costs ~10–100µs; real systems create N threads once and feed them work. Every server, game engine, and research pipeline has one. Also your first taste of task-based (vs thread-based) thinking.

**How to write it perfectly:**
- Destructor IS the hard part: close, join, no lost tasks, no join-on-self. RAII applied to threads (`std::jthread` exists for a reason — use it or hand-roll joins, but reason it through).
- `std::future`/`packaged_task` plumbing: understand what each piece owns.
- Measure the scaling curve and WHY it's sublinear (memory bandwidth, false sharing on the accumulators — foreshadowing Day 18).

**Reflexes:** thread lifetime = RAII problem; "who joins this thread?" as automatic as "who deletes this pointer?"; task granularity awareness (1M tiny tasks = overhead-dominated).

**Self-test:** TSan clean; pool destructs cleanly mid-flight; submit-from-a-task doesn't deadlock (or is documented as forbidden).

**Read:** Williams ch. 9 or Sean Parent "Better Code: Concurrency" (talk).

---

## Day 17 — Atomics I: counters, spinlock, and the memory model

**Build:** three small things. (1) Race demo: `int` counter incremented by 8 threads — watch it lose updates; fix with `std::atomic<int>`, then with `fetch_add(1, std::memory_order_relaxed)`. (2) A `Spinlock` (`atomic_flag`, `test_and_set(acquire)` / `clear(release)`, with a spin-wait loop). (3) Benchmark: atomic counter vs mutex counter vs sharded per-thread counters summed at the end.

**Why it exists:** atomics are the foundation under every lock and lock-free structure. The memory model (relaxed/acquire/release/seq_cst) is the most-asked advanced topic in HFT interviews, and you own a project (ring buffer) whose correctness depends on it.

**How to write it perfectly:**
- Learn the model as: **release = publish** (everything I wrote before is visible to whoever acquires), **acquire = subscribe**. Relaxed = atomicity only, no ordering. seq_cst = global total order, costliest.
- Spinlock spin: `while (flag.test_and_set(acquire)) { while (flag.test(relaxed)) pause(); }` — test-and-test-and-set, and know why (cache-line ping-pong).
- Sharded counters win big → contention itself is the enemy, not the primitive.

**Reflexes:** shared mutable data without a mutex → "which memory orders, and can I even justify them?"; default seq_cst until you can PROVE a weaker order (then comment the proof); contention → "can each thread have its own?"

**Self-test:** explain out loud, no notes: why does your ring buffer's producer store the tail with release and the consumer load with acquire? (This exact question comes up in interviews.)

**Read:** Williams ch. 5 (the essential chapter); Herb Sutter "atomic<> Weapons" part 1 (talk) if you want it deeper.

---

## Day 18 — SPSC ring buffer in C++ + false sharing

**Build:** the C++ SPSC ring buffer — your Go project reborn: monotonic `atomic<size_t>` head/tail, power-of-two capacity (Day 6), producer: load head relaxed + tail acquire... actually derive the orders yourself; `alignas(64)` on producer and consumer index groups. Then the experiment: benchmark WITH and WITHOUT the alignas padding, 2 threads, measure ops/sec.

**Why it exists:** this is your flagship project — now in the interview language. And false sharing (two threads writing different variables in the same 64-byte cache line, forcing the line to ping-pong between cores) is the #1 silent performance killer in threaded code; the padding demo makes it visceral.

**How to write it perfectly:**
- Producer: reads its own tail (relaxed — only it writes it), reads consumer head (acquire), writes slot, publishes tail (release). Mirror for consumer. Write the WHY of each order as comments — those comments are your interview script.
- Cache both opposite indices locally to avoid re-reading (the classic optimization — check your Go version).
- `static_assert(std::atomic<size_t>::is_always_lock_free)`.

**Reflexes:** any two atomics written by different threads → "same cache line?" `alignas(64)` on principle; "who writes this variable, who reads it" table drawn BEFORE choosing memory orders.

**Self-test:** TSan clean; padding-off vs padding-on shows a real gap (often 2–5×); numbers go into your resume repo if they beat the temp-repo version.

**Read:** your own `temp-repo/C++_code/ringbuffer.hpp` critically — find one thing to improve in it; Williams ch. 7 intro.

---

## Day 19 — Seqlock: lock-free reads of shared state

**Build:** a `Seqlock<T>` for a small POD `T` (e.g. a market-data snapshot: bid, ask, last, qty): writer increments a version counter (odd = writing), writes data, increments again (even); readers read version-data-version and retry if versions differ or are odd. Single writer, many readers. Benchmark read throughput vs a mutex-protected copy at 1 writer + 7 readers.

**Why it exists:** the canonical "publish market data" primitive — writers never block, readers never block writers, reads are wait-free when uncontended. Used in the Linux kernel, and in basically every market-data distribution path in trading. Small, elegant, and a fantastic interview story about memory ordering.

**How to write it perfectly:**
- The version counter: acquire on reads, release on the final write; the data copy itself needs `std::atomic_ref` or memcpy-style handling to dodge formal data-race UB — understand the issue, implement pragmatically, note the caveat in comments (this nuance impresses interviewers).
- T must be trivially copyable (`static_assert(std::is_trivially_copyable_v<T>)`) — readers may observe torn state mid-copy and must throw it away.
- Retry loop with a bounded spin + pause instruction.

**Reflexes:** reader/writer asymmetry as a design axis ("who's hot? optimize their path"); version counters as a general lock-free pattern; trivially-copyable as a first-class type property.

**Self-test:** torn-read hunt: make T large (256 bytes of pattern data), verify readers NEVER observe a mixed pattern across 100M reads.

**Read:** search "seqlock C++ memory ordering" (Hans Boehm's paper "Can seqlocks get along with programming language memory models" — skim the problem statement).

---

## Day 20 — Lock-free MPMC-lite: the Vyukov bounded queue

**Build:** Dmitry Vyukov's bounded MPMC queue: each slot carries its own `atomic<size_t>` sequence number; producers/consumers CAS a global ticket and wait for their slot's sequence. Implement from the algorithm description (linked below), then test with 4 producers + 4 consumers, 10M items, sum verification.

**Why it exists:** the honest step past SPSC — multiple producers/consumers without locks. Vyukov's design is the industry standard (used everywhere from Rust's crossbeam to game engines). Also your first CAS loop, the fundamental lock-free building block.

**How to write it perfectly:**
- `compare_exchange_weak` in a loop — understand weak-vs-strong and why weak+loop is idiomatic.
- Per-slot sequence numbers are the insight: they encode "whose turn is it" without a global lock. Walk through 2 producers racing on paper BEFORE coding.
- Don't invent your own algorithm today. Implementing a proven one faithfully is the skill; inventing lock-free algorithms is a research career.

**Reflexes:** CAS-loop as muscle memory (`expected` reload semantics!); paper-first for any lock-free work; deep respect — "do I actually need lock-free here, or is Day 15's queue fine?" (usually it's fine).

**Self-test:** TSan clean, exact item accounting at 8 threads, and no livelock under `taskset`-style single-core execution (or just high oversubscription on Windows).

**Read:** Vyukov's "Bounded MPMC queue" page (1024cores.net) — the algorithm source of truth.

---

## Day 21 — Consolidation + pipeline capstone

**Build (75 min):** a 3-stage pipeline: stage 1 generates synthetic order events → SPSC ring (Day 18) → stage 2 "risk-checks" them → Vyukov queue (Day 20) → stage 3 pool of 2 workers updates Day 14's order book (each worker owns disjoint symbols — no sharing = no locks; partitioning IS the best concurrency strategy). End-to-end throughput number.

**Why:** composition is the real test. Partition-by-key (sharding) is how actual exchanges scale — symbols don't interact, so don't share them.

**Rebuild hour (45 min):** blank-file: SPSC ring with memory-order comments (25 min), spinlock (5 min), condvar queue skeleton (15 min).

**Reflexes check-in:** predicate-with-wait automatic; writer/reader tables before memory orders; "can I partition instead of share?" as the FIRST concurrency question, not the last.

**Read:** `MISTAKES.md` update; skim Martin Thompson "Mechanical Sympathy" blog archive — pick any two posts.
