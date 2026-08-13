# Day 8 — FlatMap (sorted vector as a map)

Built a `FlatMap<K,V>` on top of `std::vector<std::pair<K,V>>` instead of
a tree or hash table. Idea: for a lot of real workloads (especially small
ones), a sorted contiguous array beats `std::map` even though `std::map`
has better Big-O, because `std::map` is a red-black tree where every node
is its own heap allocation, so every lookup chases pointers around memory
and eats a cache miss per level. A flat vector stays in one block, so
even though binary search is O(log n) either way, the constant factor is
way smaller because memory access is sequential/nearby instead of
scattered.

## `std::lower_bound` instead of writing my own binary search

Didn't write binary search by hand, used `std::lower_bound` with a
comparator instead. Point of doing it this way: hand-rolled binary search
is a classic place people introduce off-by-one bugs, and knowing when to
reach for the standard library instead of reinventing it is its own skill,
not just "knowing algorithms."

Needed a custom comparator because `data_` holds `pair<K,V>` but I'm
searching with just a `K` — those are different types, so plain `<`
doesn't know how to compare them. The comparator's whole job: "look only
at the pair's key, ignore the value, compare that to the search key."

```cpp
auto comp = [](const std::pair<K,V>& element, const K& k){
    return element.first < k;
};
```

`lower_bound` returns the position of the first element whose key is
`>=` what I'm searching for — NOT whether it found an exact match. That's
a separate check I have to do myself afterward (`it != end() && it->first
== key`). If it's not an exact match, that same position is exactly where
the key would need to be inserted to keep things sorted — one lookup,
two uses (find AND insert location).

## Mistakes I made along the way

- First attempt had actual executable code (the lambda + lower_bound call)
  sitting directly in the class body, outside any function. Doesn't
  compile — a class body can only hold declarations, there's no "just
  runs" code path outside a function.
- Returned the iterator itself instead of a pointer to the value
  (`return it;` instead of `return &(it->second);`) — wrong type
  entirely, iterator isn't a pointer to just the value.
- In `operator[]`'s found branch wrote `return *ite->second;` — extra
  `*` that doesn't belong, since `->second` already gives a real
  reference, not something that needs dereferencing again.
- Small early confusion about why the comparator needs 2 args and why it
  goes 4th into `lower_bound` — cleared up once I thought of it as "here's
  the row to search, here's what I'm looking for, here's the rule for how
  to compare."

## Benchmark

Built a 3-way benchmark: FlatMap vs `std::map` vs `std::unordered_map`,
at n = 100 / 10k / 1M. Did the hygiene right: -O2, no sanitizers, warm-up
pass before timing, randomized lookup order kept separate from insertion
order (so nothing gets an unfair cache-friendly pass), fixed RNG seed for
reproducibility, `volatile sink` so the compiler can't just delete the
loop as "useless work" since nothing reads the result otherwise.

Results at n=1,000,000: FlatMap ~119ms, std::map ~520ms, unordered_map
~49ms. So at a million elements, tree-based std::map is clearly the
slowest of the three, unordered_map wins outright, and FlatMap sits in
the middle — a real, visible demonstration of the "Big-O isn't the whole
story, cache layout matters" idea from the reading.

## Caught, not yet fixed

- `sink` overflowed at n=1,000,000 (ended up negative, which is impossible
  for a sum of non-negative numbers) — used `long` instead of a
  guaranteed-64-bit `int64_t`, so it silently wrapped. Matters because
  sink isn't just decoration, it's the only thing proving the loop
  actually did real work — if it overflows I can't fully trust the loop
  ran correctly, which means I shouldn't fully trust the timing either.
- n=100 and n=10,000 timings printed as basically 0ms / noise-level
  numbers — too fast for one pass to be measurable against clock jitter
  and scheduling noise. Printing in microseconds instead of milliseconds
  helps display precision but doesn't fix the real issue, which is that
  the actual measured duration is small relative to normal system noise.
  Real fix is repeating the timed loop N times and dividing, so the fixed
  noise becomes a negligible fraction of a much longer total.

## Stopped here

Didn't get to: fix sink to int64_t and confirm no more overflow at
n=1M, add repeat-loops for the small sizes to get trustworthy numbers,
and actually write down the crossover point where std::map starts beating
FlatMap (if it does at all in my data). Left as an open item, same as
Day 3 and Day 7's loose ends.