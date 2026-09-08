# Day 9 - Arena allocator (bump allocator)

Today I built an Arena, which is basically one big malloc'd block of raw
memory that hands out slices of itself by just moving a pointer forward
each time, instead of doing all the bookkeeping a normal malloc/new does.

## Why this exists

Normal malloc has to manage one shared pool for the whole program, with
allocations and frees coming in any order, possibly from multiple threads.
That means free lists, maybe locking, and fragmentation handling, all paid
on every single call whether I need it or not. An arena skips almost all
of that by assuming a much simpler pattern: a group of objects that all
get created together and all get thrown away together, like everything
allocated during one game frame or one server request. If that is true,
I do not need per object bookkeeping at all. I just bump a pointer forward
for each allocation, and when the whole group is done I reset the pointer
back to the start, which discards everything at once, instantly.

## Alignment

The hardest part conceptually was alignment. Some types need to start at
addresses that are multiples of their alignment requirement, like a double
needing to start at a multiple of 8. If I hand out a pointer that does not
satisfy that, it is undefined behavior, meaning the standard makes no
promises at all about what happens, it could work fine on my machine and
break somewhere else.

The formula I used:

```
aligned = (current + align - 1) & ~(align - 1)
```

I traced this by hand with current = 13 and align = 8. First I got the
math wrong, I thought ~(align-1) just flips the bits I wrote down, like
turning 111 into 000. That is wrong. ~ flips every bit across the whole
number, so ~7 is not 000, it is all 1s except the bottom 3 bits, which
become 0. That mask, when ANDed with anything, clears out the low bits,
which is what rounds a number down to a multiple of 8.

So the formula is really two steps. First, adding align-1 pushes current
up enough that when I round down with the mask, I land at or above where
I started instead of below it. If current was already aligned, adding
align-1 and rounding down brings it right back to itself, no padding
wasted. If it was not aligned, this pushes it into the next valid slot.
With current=13, align=8, I got aligned=16, which is the smallest multiple
of 8 that is still >= 13. The 3 bytes between 13 and 16 just become
padding, wasted space nobody uses, which is the real cost you pay for
alignment.

## Placement new

Normal new does two things, it allocates memory and runs the constructor.
Placement new only does the second part, it constructs an object at an
address I already have, using `new (raw) T(args...)`. This matters here
because the arena's allocate function only hands back raw untyped bytes,
it does not know or care what type will live there. The actual typing
happens later, per call, through placement new.

This raises the question of who calls the destructor, since I never call
delete on these objects, there is no matching heap allocation to free.
For today I went with the policy of only storing trivially destructible
types in the arena (plain structs with no owned resources), so I do not
need to manually call destructors before reset, there is nothing to clean
up. If I ever wanted to store types that own real resources, I would need
to manually call `obj->~T()` on each one before reset.

## The bug I hit

In allocate(), after computing the aligned pointer and the padding, I
wrote:

```
offset_ = padding + size;
```

This should have been `offset_ += padding + size;`. With plain =, the
first call happened to look correct since offset_ started at 0, but every
call after that reset offset_ back down to just that one allocation's
size instead of adding onto the running total. That means almost every
allocation after the first one was overlapping memory I had already
handed out to a previous object, silently overwriting earlier objects
instead of giving each one its own separate slice.

The scary part is my benchmark did not catch this on its own. It only
measured how fast the loop ran, it never read back the objects afterward
to check their values were still correct. A bug that puts objects in the
wrong place is invisible if nothing ever checks where they actually
ended up. This is the same lesson as earlier days, a fast wrong answer
is still wrong, so timing alone is never proof of correctness.

I fixed the line to use += instead of =. Still need to actually run a
verification loop that constructs all million objects, keeps their
pointers, and reads them back afterward to confirm none of them got
overwritten, before I fully trust the fix or the benchmark number.

## Numbers before the fix

new/delete: 45.1368 ms
arena: 1.20984 ms
speedup: 37.3x

This number is suspicious in hindsight. It was faster than it should have
been because the buggy version was not actually touching a full million
distinct objects worth of memory, it kept overlapping the same small
region over and over, which is a hint I should have caught earlier from
the number itself being unusually good.

## Numbers after the fix

new/delete: 42.1202 ms
arena: 1.38486 ms
speedup: 30.4x

Still clears the 10x bar comfortably, but it is notably lower than the
buggy version's 37.3x. That drop actually makes sense and is a good sign,
not a bad one. The buggy version was artificially fast because it kept
overwriting the same small sliver of memory instead of touching a full
million objects worth of space. The fixed version does genuinely more
real work, touching a full million distinct slots, so being somewhat
slower than the broken version is exactly what I would expect.

## Still open

I still have not run the actual correctness verification loop, meaning
constructing all million objects, keeping their pointers, and reading
every one back afterward to confirm none of them got overwritten. The new
timing number is more believable than the old one, but a believable
number is still not proof.