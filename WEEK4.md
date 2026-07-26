# Week 4 — Low Latency & The Capstone (Days 22–28)

Theme: zero-cost abstraction, measurement discipline, and assembling everything into one system you can demo and defend in any interview.

---

## Day 22 — Benchmark harness (your own nano-google-benchmark)

**Build:** a tiny benchmarking library you'll reuse forever: `BENCH(name, iterations, lambda)` that does warm-up runs, N timed runs, reports min/median/P99 (min matters most for microbenchmarks — it's the least-noise sample), a `do_not_optimize(x)` escape hatch, and cycles-per-op via `std::chrono::steady_clock` (and `__rdtsc` if you want). Validate it by re-measuring 3 known quantities: L1 hit vs main-memory miss (pointer-chase a shuffled vs sequential array), branch predicted vs mispredicted (sorted vs unsorted branchy sum), and function call vs inlined.

**Why it exists:** every claim you've made on your resume ("4M ops/sec", "25% lower latency") rests on measurement methodology, and interviewers probe exactly that. Bad benchmarks (dead-code elimination, cold caches, frequency scaling) are how people lie to themselves. Owning a harness means owning the discipline.

**How to write it perfectly:**
- `do_not_optimize`: `asm volatile("" : : "g"(&x) : "memory")` — understand WHAT it tells the compiler (pretend x is read via unknown means).
- Warm-up before measuring; interleave A/B runs to distribute thermal/frequency drift; report distribution, never a single number.
- The three validation experiments must reproduce known physics: ~100× for cache miss chase, big gap for mispredicted branches. If they don't, your harness is broken — fix it before trusting it.

**Reflexes:** distrust of any single-number benchmark (yours included); "was the work optimized away?" as the first question about any surprising result; knowing your machine's actual numbers (YOUR L1 latency, YOUR mutex cost) not just the folklore table.

**Self-test:** re-benchmark Day 18's ring buffer with the new harness; do the numbers hold?

**Read:** Chandler Carruth "Tuning C++: Benchmarks, and CPUs, and Compilers! Oh My!" (CppCon 2015) — the do_not_optimize talk.

---

## Day 23 — CRTP & compile-time polymorphism

**Build:** a strategy-pattern order-validator two ways. (1) Classic: `struct IValidator { virtual bool check(const Order&) = 0; }` with 3 implementations called through a base pointer. (2) CRTP: `template<class Derived> struct Validator { bool check(const Order& o) { return static_cast<Derived*>(this)->check_impl(o); } }` — same 3 strategies, resolved at compile time. Benchmark both over 100M orders with your Day 22 harness; inspect both on godbolt.

**Why it exists:** virtual dispatch costs an indirect call, blocks inlining, and adds a vtable pointer per object — usually irrelevant, but in a hot loop processing millions of messages it's real. CRTP (and templates generally) gives you the Strategy pattern with zero runtime cost — the signature C++ trick that Java/Go can't express. Every serious C++ library (Eigen, CppCon codebases, trading frameworks) uses it.

**How to write it perfectly:**
- Understand the actual costs: it's less the indirect call itself and more the LOST INLINING and what inlining enables (constant folding, vectorization). Look at the godbolt output: the CRTP version's loop often compiles to almost nothing.
- Also implement dispatch a third way: `std::variant<V1,V2,V3>` + `std::visit` — the modern middle ground (closed set of types, no inheritance, still fast).
- Know when NOT to: virtual is fine for cold paths, plugin boundaries, true open sets. CRTP infects signatures with templates — a real cost in compile time and ergonomics.

**Reflexes:** "is this dispatch hot?" before choosing virtual/CRTP/variant; reading godbolt output as routine, not as an event; abstraction cost as a measured quantity, not an ideology.

**Self-test:** explain out loud the three dispatch options and their trade-off triangle (flexibility / performance / ergonomics).

**Read:** Meyers Item 41-ish region; search "CRTP explained" (Fluent C++ series is good).

---

## Day 24 — Type erasure: build your own `std::function`

**Build:** `Function<R(Args...)>` — a type-erased callable: small-buffer optimization for small callables (Day 5 skill reused!), heap fallback for large ones, manual vtable (function pointers for invoke/destroy/move) or a templated concept-model pair. Store lambdas with captures, free functions, functors; call them uniformly. Compare overhead vs raw lambda call and vs `std::function` with the Day 22 harness.

**Why it exists:** type erasure is the pattern behind `std::function`, `std::any`, `std::shared_ptr`'s deleter — runtime polymorphism WITHOUT inheritance, across unrelated types. Building it teaches you: how vtables actually work (you make one by hand), SBO again, and exactly what that innocent `std::function` in your thread pool costs (allocation + indirect call) — which is why Day 16's pool noted it as an accepted cost.

**How to write it perfectly:**
- Manual vtable version teaches the most: a static `struct VTable { R(*invoke)(void*, Args...); void(*destroy)(void*); void(*move)(void*, void*); }` instantiated once per erased type via a template — stare at this until it clicks; it demystifies ALL of C++'s runtime polymorphism.
- SBO decision: buffer of 24–32 bytes, `if constexpr (sizeof(F) <= BufSize && std::is_nothrow_move_constructible_v<F>)`.
- This is the hardest build of the month. If it takes 3 hours, fine. If you finish only the heap version without SBO, also fine — add SBO as a stretch.

**Reflexes:** seeing `std::function` costs (possible allocation, no inlining) automatically → use `auto`/templates for hot callbacks, `Function` only at true type-erasure boundaries; "concept-model" as a recognized idiom in other people's code.

**Self-test:** your Function passes: capture-heavy lambda, mutable lambda, function pointer; ASan clean under move/destroy churn.

**Read:** Sean Parent "Inheritance Is The Base Class of Evil" (13 min, the type-erasure gospel talk).

---

## Day 25 — Binary protocol: encode/decode your own ITCH-style feed

**Build:** define a tiny binary protocol — 3 message types (AddOrder, Cancel, Trade) with fixed layouts, big-endian on the wire: a `Writer` that serializes 10M random messages into one buffer, and a `Reader` that parses them back (Day 12 zero-copy style: return views/structs referencing the buffer) and rebuilds order counts. Measure msgs/sec each way. Add a fuzz check: flip random bytes, reader must never crash or overread (bounds-check every length).

**Why it exists:** binary protocols are the lingua franca of trading (ITCH, OUCH, SBE), games, and databases. You're doing this against real ITCH in the LOB project — today is the from-scratch version that makes you understand every design decision: fixed layouts = no parsing ambiguity, length prefixes = framing, endianness = portability.

**How to write it perfectly:**
- Read integers with `memcpy` into the typed variable then byteswap — NOT by casting the buffer pointer (`*(uint32_t*)p` is UB via strict aliasing and may fault on unaligned addresses). `std::byteswap` (C++23) or hand-rolled.
- One `read_u16/u32/u64` helper trio, bounds-checked, used everywhere — parsing code full of raw offsets is where security bugs live.
- Struct layout on the wire ≠ struct layout in memory: never `memcpy` a whole struct across the wire boundary without `#pragma pack` understanding — prefer field-by-field with helpers.

**Reflexes:** `*(T*)buffer` = instant red flag (aliasing/alignment); every read bounds-checked against buffer end; "wire format" and "memory format" as consciously separate things.

**Self-test:** round-trip 10M messages bit-exact; fuzz loop (10k random corruptions) with zero crashes under ASan.

**Read:** your `lob-repo/src/itch_messages.hpp` with today's eyes — find and fix any casting/aliasing sins; skim the SBE (Simple Binary Encoding) design rationale.

---

## Day 26 — Low-latency logger

**Build:** a logger where the hot thread does near-zero work: hot path writes a fixed-size binary record (timestamp via `steady_clock`/rdtsc, log-site id, up to 3 uint64 args — NO formatting, NO allocation) into your Day 18 SPSC ring; a background thread drains the ring, formats to text, writes to file. Compare hot-path cost vs `printf`/`std::cout` and vs formatting-then-queueing, with the Day 22 harness. Handle ring-full policy explicitly (drop + count, or overwrite — document the choice).

**Why it exists:** logging is the classic latency killer — formatting and I/O on the critical path costs microseconds where the budget is nanoseconds. Every trading system, and increasingly every game engine, uses exactly this architecture (deferred formatting). It's also the perfect capstone REUSE: your own ring buffer becomes infrastructure for your own tools.

**How to write it perfectly:**
- The hot path is ~20ns: one ring slot write. Formatting happens where latency is free (background thread).
- Log-site id → format string mapping: a static registry (the site registers "order %d filled qty %d" once, hot path sends just the id + args). This is how NanoLog works.
- Timestamping: know steady_clock's resolution and cost on your machine (measure it — Day 22 harness).
- Backpressure policy is a DESIGN DECISION: dropping logs silently is bad, blocking the hot path is worse — count drops and log the count.

**Reflexes:** "what does the critical path pay for this?" for every cross-cutting concern (logging, metrics, error handling); deferred/offloaded work as the default answer; explicit backpressure policy on every queue you ever create.

**Self-test:** hot-path cost < 50ns/record measured; zero records lost at moderate rate; drop counter works when you flood it.

**Read:** NanoLog paper abstract + design section (Stanford, "NanoLog: A Nanosecond Scale Logging System") — skim, you just built the core idea.

---

## Day 27 — Capstone part 1: the mini trading system

**Build (this is 2h today + 2h tomorrow):** assemble `minitrade/` from your month's parts: feed generator thread producing Day 25 binary messages → Day 18 SPSC ring → parser thread (Day 12/25 zero-copy) → Day 14 order book (Pool + HashMap + FlatMap + intrusive lists) now WITH matching (price-time priority, partial fills — port your LOB logic), publishing top-of-book via Day 19's Seqlock → a strategy thread reading the Seqlock snapshot and "trading" a naive spread-crossing rule, all instrumented with Day 26's logger and Day 22's harness for end-to-end tick-to-trade latency (timestamp at generation, timestamp at strategy decision, histogram the difference).

Today: wire feed → parser → book, get correct matching, logger integrated.

**Why it exists:** this IS a trading system's skeleton — feed handler, book builder, market-data distribution, strategy, telemetry. Firms pay for exactly this shape. One repo demonstrating every skill from the month, with a latency histogram as the money shot.

**How to write it perfectly:**
- Resist scope: 3 message types, 1 symbol is fine, naive strategy. The ARCHITECTURE is the deliverable.
- Each stage owns its data; hand-offs only through your queues (ownership transfers, document each).
- Keep every component in its own header — this repo doubles as your personal library.

---

## Day 28 — Capstone part 2: measure, harden, tell the story

**Build (90 min):** finish minitrade: tick-to-trade latency histogram (P50/P99/max), find your biggest latency contributor (timestamp between stages), improve ONE thing (e.g. batch parsing, or move a copy), re-measure, keep the before/after numbers. TSan + ASan full passes.

**Write (30 min):** `minitrade/README.md` — architecture diagram (ASCII fine), per-component one-liner linking back to which day built it, the latency histogram, the one optimization story (before/after). Push everything; pin the dojo repo on GitHub.

**The final reflexes check — you are n1-track when ALL of these are automatic:**
1. Blank `.cpp` file → class with invariants, Rule of Zero, const-correct, in one pass without thinking.
2. Any bug report → "ASan, TSan, or UBSan first" before print-debugging.
3. Any perf question → "measured with what? distribution or single number?"
4. Any shared state → owner/writer/reader table before code.
5. Any abstraction → "what does it cost, and did I check godbolt?"
6. Any container/allocation → access pattern and lifetime group named out loud.

**After day 28:** the loop continues at lower intensity — one dojo-style build per week (pick from: memory-mapped file KV store, epoll/IOCP echo server, coroutine generator, SIMD CSV parser, timer wheel, hazard pointers), keep the daily 10-minute pro-code reading, keep `MISTAKES.md`. And take the minitrade + LOB + ring buffer trio into every interview — you now have a story for every systems question they can ask.

**Read:** nothing new. Reread your entire `MISTAKES.md` start to finish — that document is the month's real diploma.
