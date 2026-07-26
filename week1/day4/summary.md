# Day 4 — UniquePtr<T>: ownership as a type

Built a `UniquePtr<T>` — the idea being that ownership becomes a property
of the TYPE, checkable by the compiler at compile time, instead of
something you just have to remember. If you hold a `UniquePtr<Node>`, that
fact alone means "I am the sole owner, I delete this when I die."

## Structure

```cpp
explicit UniquePtr(T* ptr = nullptr);
~UniquePtr();
UniquePtr(const UniquePtr&) = delete;
UniquePtr& operator=(const UniquePtr&) = delete;
UniquePtr(UniquePtr&& other) noexcept;
UniquePtr& operator=(UniquePtr&& other) noexcept;
T& operator*() const;
T* operator->() const;
T* get() const;
T* release();
void reset(T* new_ptr = nullptr);
```

`explicit` on the constructor stops implicit conversions from a raw
pointer into an owning `UniquePtr` — without it, passing a raw pointer
into a function expecting a `UniquePtr` could silently transfer ownership
with zero visible syntax marking that it happened. `explicit` forces the
transfer to be written out, visible in the source.

`= delete` on copy ctor/assignment isn't just "don't define these" — it's
a real compile-time error the moment anyone tries to call them. Better
than the old pre-C++11 trick (private + undefined), since that could fail
at LINK time instead if a member/friend function called it internally,
which is a much worse error to debug.

## Move ctor/assignment

Same steal-and-null pattern as Vec's move logic from Day 2, just one
pointer instead of three members. Move assignment needs a manual
`this != &other` self-check since there's no copy constructor to build a
by-value parameter from — copy-and-swap doesn't apply here.

## release() vs reset() vs destructor

Three different disown/dispose operations, easy to conflate:

- destructor: deletes unconditionally, always.
- `release()`: hands the raw pointer back to the caller WITHOUT deleting —
  caller now owns it.
- `reset(new_ptr)`: has to handle `p.reset(p.get())` — saving the OLD
  pointer first, reassigning `ptr_ = new_ptr`, THEN deleting the old one,
  same "don't destroy before the new state is secured" order as Day 1's
  `grow_`. Deleting first would null out your own member right after
  storing a dangling address back into it.

Caught a real bug myself in `operator*`: had it returning `ptr_` (a `T*`)
from a function declared to return `T&` — needed `return *ptr_;` to
actually dereference. Slipped through untested because the linked-list
demo below never happened to call `operator*` directly, only `->` and
`.get()`.

## make_unique + linked list test

```cpp
template <typename T, typename... Args>
UniquePtr<T> make_unique(Args&&... args) {
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}
```

Built a `Node<T>` with a `UniquePtr<Node> next` member, chained three nodes
together with zero naked `new`/`delete` anywhere in `main`. Confirmed
`UniquePtr<Node<int>> copy = head;` is a compile error (the `= delete`
doing its job). The actual payoff: destroying `head` cascades — its
destructor deletes node 1, which destroys ITS `next` member, deleting node
2, and so on down the whole list automatically. One destructor call chain,
zero manual bookkeeping.

Still need to actually verify on godbolt.org: compare `-O2` assembly of
`UniquePtr::operator*` against a raw pointer dereference, to see the
zero-cost claim confirmed in real generated instructions.