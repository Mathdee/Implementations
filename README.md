# C++ Dojo — 28 Days to Real Instincts

**Goal:** go from "LeetCode C++" to an engineer who writes correct, fast, idiomatic C++ from a blank file, with automatic reflexes. Every day you build something real from scratch in ~2 hours.

## How to use this

- Open ONLY today's week file (`WEEK1.md` … `WEEK4.md`). Each day has: **Build** (the task), **Why it exists** (use case), **How to write it perfectly**, **Reflexes** (the instincts to burn in), and **Self-test** (prove you learned it).
- One folder per day inside a `dojo/` repo: `day01_vec/`, `day02_string/`, … Each compiles standalone with `g++ -std=c++20 -Wall -Wextra -fsanitize=address,undefined -g`.
- **Type every line yourself.** AI may only: explain a concept, review your finished code, generate test inputs. If AI writes the code, the day didn't happen.
- **The rebuild rule:** each day starts with 10 minutes rebuilding yesterday's core from memory (just the skeleton, not the whole thing). This is how reflexes form.
- Don't gold-plate. 2 hours means a working, tested core — not every edge case.

## Non-negotiable compiler setup

All warnings, always sanitizers during development:

```
g++ -std=c++20 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -g main.cpp
```

For benchmarking days: `-O2 -march=native` and sanitizers OFF (they distort timings 10–50×).

## The 10 master reflexes (you'll internalize all of them by day 28)

1. **Who owns this?** Every resource has exactly one owner. If you can't answer, the design is wrong. (RAII)
2. **Rule of Zero first.** Write no destructor/copy/move if members manage themselves. Only drop to Rule of Five when you hold a raw resource.
3. **`const` by default.** Variables, member functions, references — non-const is the exception you justify.
4. **Value semantics by default;** pass big things by `const&`, sink parameters by value + move, never return by pointer when a value works.
5. **Ban naked `new`/`delete`.** `unique_ptr`, containers, or an arena — always a named owner.
6. **Braces initialize, `=` assigns.** Use `{}` init; know that `std::vector<int> v(5)` and `{5}` differ.
7. **Invariants live in the class.** Constructors establish them, every method preserves them, asserts document them.
8. **Measure, never guess.** No performance claim without a benchmark; know your costs table (L1 ~1ns, main memory ~100ns, mutex ~20ns uncontended, syscall ~1µs).
9. **The compiler is your test suite #0.** If it can be a compile error instead of a runtime error (enum class, strong types, `constexpr`, concepts), make it one.
10. **Read the assembly when in doubt.** godbolt.org is a tab you always have open.

## Design patterns you'll actually use (learned by building, not memorizing)

- **RAII** (days 1–4) — the pattern that IS C++.
- **Iterator** (day 3, 8) — how your types plug into the STL and range-for.
- **Strategy via templates / CRTP** (day 16–17) — zero-cost polymorphism, the low-latency alternative to virtual.
- **Type erasure** (day 17) — how `std::function` works; runtime flexibility with a controlled cost.
- **Object Pool / Arena** (days 9–10) — the allocation patterns every trading system uses.
- **Producer/Consumer** (days 15, 19–21) — queues, condition variables, lock-free.
- **State machine via `std::variant` + visitor** (day 18) — modern replacement for enum+switch spaghetti.
- **Factory & Pimpl** appear in passing where they genuinely help; you'll learn to distrust pattern-for-pattern's-sake.

## Weekly themes

| Week | File | Theme |
|---|---|---|
| 1 (days 1–7) | WEEK1.md | Ownership & value semantics — rebuild the std library's core yourself |
| 2 (days 8–14) | WEEK2.md | Data structures & memory — cache-aware containers, allocators, parsing |
| 3 (days 15–21) | WEEK3.md | Concurrency — threads, locks, atomics, lock-free |
| 4 (days 22–28) | WEEK4.md | Low latency & the capstone — benchmarking, zero-cost abstraction, a mini trading system |

## Support reading (15–20 min/day, listed per-day in the week files)

Backbone sources: learncpp.com (fundamentals), cppreference.com (the only API reference you should use), "C++ Core Guidelines" (skim rules as they come up), CppCon talks on YouTube (assigned on specific days). If you buy one book: **"Effective Modern C++" (Scott Meyers)** — items are assigned per-day.

## The extra thing you asked for ("if I need something else")

Being n1 isn't just code. Three habits, 10 minutes total per day, all four weeks:

1. **Read one piece of great code daily** (10 min): rotate through `folly/` (Facebook), `abseil-cpp/` (Google), and LMAX Disruptor's design docs. Reading pros rewires your defaults faster than writing alone.
2. **Keep `MISTAKES.md`** in the dojo repo: every bug, one line — what you assumed, what was true. Reread it Sundays. Your personal anti-pattern list becomes your fastest-growing asset.
3. **Explain out loud.** After each build, 60 seconds, out loud, no notes: what you built and the one non-obvious decision. This is literally interview training.
