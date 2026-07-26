# Day 3 — Templates + iterators (partial)

Converted `Vec` from `int`-only to `template<typename T> class Vec`. Every
method got `template <typename T>` prefixed and `Vec<T>::` scoping when
defined outside the class. Rule of Five from Day 2 carried over unchanged,
just swapped `int`/`int*` for `T`/`T*` everywhere (including `grow_`,
`push_back`, `operator[]`).

Added `begin()`/`end()`:

```cpp
T* begin() { return data_; }
T* end()   { return data_ + size_; }
```

Raw pointers are valid iterators for a contiguous container like this —
`begin` is the first element, `end` is one-PAST the last valid element
(`data_ + size_`, not `+ capacity_`, since you only want to iterate real
elements, not unused allocated space).

## Left open / not finished

Got asked WHY templates have to live entirely in headers (as opposed to
declaration in `.h` + definition in `.cpp`, like normal functions) —
didn't get to answer this properly before moving on. The gist as I
understand it so far: normal function definitions just need to exist
SOMEWHERE for the linker to find later. Templates are different because
the compiler doesn't generate real code for `Vec<T>` in the abstract — it
needs to instantiate `Vec<int>`, `Vec<std::string>`, etc. as actual
concrete types at COMPILE time, wherever they're used. If the definition
is hidden in a separate .cpp the compiler can't see when compiling
`main.cpp`, there's nothing to instantiate from — need to come back to
this properly before splitting any project into header/source files.

Still not done from the original plan:

- `std::sort` / `std::find` verification (should just work given begin/end
  are valid iterators, but never actually ran the test)
- `emplace_back` (spec says `push_back(T(args...))` is an acceptable
  stand-in for today, real placement-new version is Day 9)
- `Vec<std::string>` with 100k strings under ASan self-test
- reading template compiler errors bottom-up ("required from here")

To revisit.