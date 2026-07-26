# Day 7 — Consolidation + Matrix mini-capstone

Built a `Matrix` class on top of my own `Vec<double>`,single flat array,
`operator()(row, col)` doing `row * cols + col` to turn 2D coordinates into
a 1D index. Row-major layout: row 0's elements sit first, then row 1's,
etc, all back to back in memory.

Composition win worth noting: `Matrix` has no destructor, no copy/move
constructors of its own, and doesn't need any. It just holds a `Vec<double>`
as a member, and `Vec` already has a correct Rule of Five from Days 1-2,
so `Matrix` inherits correct resource management entirely for free, just
by containing something that already has it. Didn't expect that to just
work without writing a single extra line.

Needed a const `operator[]` on `Vec` I hadn't written before, since
`Matrix`'s const `operator()` calls into `data_[i]` on a `const Vec&`,
you can't call a non-const method through a const reference, so the const
overload was required, not optional.

## What I didn't get at first

Traced `A(i,k)` and `B(k,j)` in the naive `i,j,k` loop by hand and initially
described the memory access pattern wrong ("gets slower as k increases"),
actually walked through real numbers and saw the real pattern: `A(i,k)`
moves by +1 every step (fully sequential, cache-friendly) while `B(k,j)`
jumps by the row width every step (strided, cache-hostile), same inner
loop, two completely different access patterns for the two operands.

Reordering to `i,k,j` and hoisting `A(i,k)` out into a single register
value before the innermost loop turns the ENTIRE inner loop sequential,
both `B(k,j)` and `C(i,j)` become +1-per-step, and `A` isn't re-read from
memory at all inside that loop. Same math, same total multiply-adds,
purely different order of memory access.

Also caught my own design mistake: wrote `multiply_naive`/`multiply_reordered`
as member functions of `Matrix`, but neither one ever touches `this`,
they only use their two parameters. Should've been free functions from
the start; a method should only be a member if it actually needs the
object it's called on.

## Results

At -O3, 512x512:
- naive: ~0.7-1.5s
- reordered: ~0.07s
- speedup: 10-21x depending on the run

Bigger than the 5-10x I expected going in. Turns out it's not just cache
locality, fixing the access pattern also lets the compiler auto-vectorize
the reordered inner loop (process multiple doubles per instruction), which
wasn't possible on the naive version's strided access. So it's two wins
stacked, cache locality unlocking vectorization, not one effect alone.

Verified `C1` and `C2` (naive vs reordered result) match within a small
tolerance, not exact equality, floating point addition isn't strictly
associative, so summing the same numbers in a different order can differ
in the last bit or two. Not a bug, just floating point. Good reminder to
check outputs match before trusting a speedup number at all, a fast wrong
answer is worthless.

