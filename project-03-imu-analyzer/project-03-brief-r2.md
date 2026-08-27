# Learn C++ — Project 3 Brief: IMU Telemetry Log Analyzer

**Revision 2 — retargeted from C++17 to C++23/26.**
**Status:** Issued, not started
**Builds on:** `project-01-caesar-cipher.md`, `project-02-numeric-statistics.md`
**Deliverable (yours):** `imu_analyzer.cpp`
**Provided:** `imu_log.csv` (fixture), `toolchain_probe.cpp` (run this first)

---

# Part 0 — Standard version and toolchain

## What we're targeting and why

Your toolchain is **GCC 15.2 on Ubuntu 26.04**, which is genuinely current. We will build
with:

```bash
-std=c++26
```

But read this carefully, because the flag and the *dialect we write in* are two different
decisions:

**Compile with `-std=c++26`, write C++23 in substance.**

- **C++23** is the latest *published* ISO standard. Everything we use in this project is
  C++20 or C++23, fully implemented in GCC 15, stable, and not going to change.
- **C++26** was declared feature-complete at the March 2026 Croydon meeting but is not yet
  ISO-published. GCC's own status page says C++26 support is experimental and that "no
  attempt will be made to maintain backward compatibility with implementations of C++26
  features that do not reflect the final standard."
- The `-std=c++26` flag is a strict superset — everything C++23 works — so there is no cost
  to using it, and you get GCC 15's C++26 extras (pack indexing, `= delete("reason")`,
  variadic friends, `#embed`) if we ever want them. We won't in this project.
- The marquee C++26 features — **reflection** and **contracts** — need GCC 16 and an extra
  `-freflection` flag. They are also deep metaprogramming/design-by-contract territory,
  which is several projects away regardless.

So the flag says "latest," the code says "stable." You lose nothing.

## Two cautions specific to your goals

**Your Auburn class.** Find out what standard the course mandates before the first
assignment. Writing C++23 for our projects is fine, but discovering the autograder is on
C++11 the night before a deadline is a bad evening. If the class pins an older standard, we
keep going as-is and I'll flag which of our habits don't back-port.

**Embedded lags — badly.** "Latest C++" often does not reach a microcontroller.
`arm-none-eabi-gcc` tracks mainline GCC and is fine; ESP-IDF is reasonably current; the
Arduino AVR core is effectively stuck in the C++11/14 era. More importantly, embedded
builds frequently compile with `-fno-exceptions` and `-fno-rtti`, which **removes
`try`/`catch` entirely**. That is not a hypothetical for you — it is why §2's aside on
`std::from_chars` matters, and why `std::expected` (§8) is the error-handling mechanism the
embedded world actually converged on.

## Run this first

`toolchain_probe.cpp` is provided. It is a feature probe, not project code — it touches
every language and library feature this project needs, in a domain unrelated to IMUs.

```bash
g++ -std=c++26 -Wall -Wextra -pedantic -o probe toolchain_probe.cpp && ./probe
```

If it prints `--- all probes passed ---`, your toolchain is confirmed and you can ignore
every "requires GCC N+" caveat below. If anything fails to compile, tell me which line and
we'll adjust the spec rather than fight it.

## What changed from Revision 1

| Was (C++17) | Now |
|---|---|
| Positional aggregate init, with a warning about swapping fields | **Designated initializers** — the hazard is gone |
| `<iomanip>` with sticky stream state | **`std::format` / `std::print`** |
| `parse_row` returns `bool` + two out-params | **`std::expected<ImuReading, std::string>`** |
| `load_log` returns `bool` + three out-params | **`std::expected<LoadResult, std::string>`**, with a struct |
| `static_cast<int>(some_enum)` | **`std::to_underlying`** |
| Manual `[0] == '#'` comment check | **`.starts_with('#')`** |
| — | `<=>`, `std::ranges::sort` with projections, `std::from_chars` all now taught |

The `std::optional`-vs-`bool`+out-param contrast from Revision 1 has been **upgraded, not
removed**: it is now `std::optional` vs `std::expected`, which is a choice between two
modern tools on their merits rather than modern-vs-legacy.

---

# Part 1 — Concepts

You answered "no idea / haven't used it" on structs, file I/O, and enums; "yes" on lambdas
and dunder overriding. So this part teaches structs, floating point, file streams,
iterators, `std::sort`'s contract, `enum class`, `std::optional`, and `std::expected` from
scratch, and treats lambdas and `operator<<` as *translations* of things you already know.

One concept you did not ask about is in here anyway: **floating point**. Projects 1 and 2
were entirely `int` and `char`. This project is `double` throughout, and doubles have
failure modes that integers do not. It is the single most likely source of bugs in what
follows.

---

## 1. `struct`

A `struct` is a bundle of named values that travels as one object. Closest Python analogue:
a `@dataclass`.

```cpp
struct Point {
    double x {};
    double y {};
};
```

The `{}` after each member is a **default member initializer** — it value-initializes to
`0.0`. Without it, `Point p;` leaves both members holding garbage, and reading garbage is
UB. Write them every time.

### Creating one

```cpp
Point a { 3.0, 4.0 };                    // aggregate init, positional, declaration order
Point b {};                              // both members zero
Point c { .x = 3.0, .y = 4.0 };          // C++20 designated initializers
Point d { .y = 4.0 };                    // x gets its default (0.0)
```

**Use the designated form.** Positional aggregate initialization is a real hazard in a
struct of seven `double`s: `Point { 4.0, 3.0 }` compiles exactly as happily as
`Point { 3.0, 4.0 }`, because the compiler cannot know you swapped them — both are
`double`. Naming the members makes the swap impossible and the code readable at the call
site. This is the "correct by construction" theme from Projects 1 and 2, available as
syntax.

Two C++ rules that differ from C, worth knowing so the compiler errors make sense:

- **Designators must appear in declaration order.** `Point { .y = 4.0, .x = 3.0 }` is a
  compile error in C++ (it is legal in C). This is a feature — it keeps the initialization
  order visible and predictable.
- You may skip members; skipped ones get their default member initializer.

### Access

```cpp
const double d { std::sqrt(a.x * a.x + a.y * a.y) };
a.x = 5.0;   // fine, a is not const
```

### `struct` vs `class`

In C++ these are the *same feature* with one difference:

| | default member access | default inheritance |
|---|---|---|
| `struct` | `public` | `public` |
| `class` | `private` | `private` |

That is the entire language-level difference. Convention: `struct` for plain data with no
invariants to protect, `class` when there is behavior and state to guard. This project uses
a `struct` because an IMU reading is just seven numbers. Classes proper — constructors,
member functions, access control, RAII of your own — are Project 4.

### Copy semantics

Assignment copies **member-wise**:

```cpp
Point p { .x = 1.0, .y = 2.0 };
Point q { p };   // independent copy
q.x = 99.0;      // p.x is still 1.0
```

The Project 1 lesson again: C++ copies, Python binds names. Your struct here is 7 doubles =
56 bytes. Not huge, not free, and copying it per-iteration over thousands of readings is
pure waste. Parameter rule from Project 1 is unchanged: **`const ImuReading&` to read,
`ImuReading&` to modify, never by value** unless you specifically want a copy.

### `std::vector<Point>` stores structs *inline*

A Python `list` of objects is an array of pointers to objects scattered across the heap.
A `std::vector<Point>` is one contiguous block of `Point`s, back to back. Iterating it
reads straight down a cache line. This is most of why C++ numeric code is fast, and why
`std::vector<T>` needs `T` to be a fixed known size.

### Embedded aside: `sizeof` and padding

`sizeof(Point)` is 16 — two 8-byte doubles, nothing wasted. But:

```cpp
struct Bad  { char c; double d; char e; };   // sizeof == 24, not 10
struct Good { double d; char c; char e; };   // sizeof == 16
```

The compiler inserts **padding** so each member sits at an address that is a multiple of
its alignment. Reordering members largest-first packs them tighter. On a desktop this is a
curiosity; on a microcontroller with 20 KB of RAM, or when a struct maps onto a register
block or a wire protocol, it is the whole ballgame. Print `sizeof(ImuReading)` at some
point in this project and confirm you get 56.

---

## 2. Floating point — the new hazard

### `double` vs `float`

`float` is 32-bit (~7 significant decimal digits), `double` is 64-bit (~15–16). **Use
`double` by default** on a desktop; the hardware does double-precision at the same speed
and `float` just costs you accuracy.

The embedded story inverts this, and it matters for you: a Cortex-M4F (STM32F4, Teensy 3.x)
has a **single-precision-only** FPU. `float` math is a single instruction; `double` math is
emulated in software and can be 10–50× slower. On a Cortex-M0 there is no FPU at all.
This is why embedded code is full of `float` and why `sinf`/`sqrtf` exist alongside
`sin`/`sqrt`. Also note `3.14` is a `double` literal — `3.14f` is the `float` one, and
forgetting the `f` silently drags a whole expression into double math.

For this project: `double` everywhere.

### `==` does not do what you want

```cpp
0.1 + 0.2 == 0.3     // false
```

`0.1` is not representable in binary floating point, any more than `1/3` is representable
in finite decimal. The stored value is a hair off, and errors accumulate.

```cpp
constexpr double tolerance { 1e-9 };
if (std::fabs(computed - expected) < tolerance) { ... }
```

**The exception that matters here:** comparing against `0.0` a value you *parsed directly
from the literal text `0.0000`* is exact and safe. No arithmetic happened, so no error
crept in. `x == 0.0` is defensible when `x` came straight from a parse; it is not
defensible when `x` came out of a subtraction. Know which one you have.

### NaN — the one that will actually bite you

`NaN` ("not a number") is a real `double` value produced by `0.0/0.0`, `std::sqrt(-1.0)`,
and — critically — **by successfully parsing the text `"nan"`**.

Its defining property: **every comparison involving NaN is false, including equality with
itself.**

```cpp
const double n { std::nan("") };
n == n     // false
n != n     // true   <- the classic NaN test
n < 1.0    // false
n > 1.0    // false  <- both false at once
```

Three consequences:

1. A single NaN in a sum makes the whole sum NaN. It propagates and poisons everything.
   Your min/max loops from Project 2 would silently produce garbage.
2. NaN is neither less than, greater than, nor equal to anything, which breaks the ordering
   rules `std::sort` requires (§5) and puts you in **undefined behavior — a real
   out-of-bounds write, not a wrong answer**.
3. Therefore NaN must be rejected **at the parse boundary**, before it reaches a vector.
   This is the Project 2 theme "library leniency is the caller's problem," with a much
   worse penalty for ignoring it.

`inf` is the same story minus the self-inequality: `1.0/0.0` is `+inf`, `"inf"` parses, and
`inf - inf` is NaN.

The check, from `<cmath>`:

```cpp
if (!std::isfinite(value)) { /* NaN or ±inf; reject */ }
```

`std::isfinite` is true only for ordinary finite numbers. It is the one test you need.

**Language-level confirmation you'll meet in §5:** when you default the `<=>` operator on a
struct containing `double`s, the compiler gives you `std::partial_ordering` rather than
`std::strong_ordering` — *specifically because of NaN*. The type system encodes this exact
hazard. That is a nice sanity check that you have understood it.

### `std::stod` and its second parameter

`std::stod` is `std::stoi`'s double sibling, with the same leniency problem you diagnosed
in Project 2:

| Call | Result |
|---|---|
| `std::stod("3.14")` | `3.14` |
| `std::stod("  3.14")` | `3.14` — leading whitespace skipped |
| `std::stod("1.2e-2")` | `0.012` — scientific notation is valid |
| `std::stod("0.0121xyz")` | **`0.0121`** — parses the prefix, no complaint |
| `std::stod("nan")` | NaN — **succeeds** |
| `std::stod("inf")` / `"-inf"` | ±inf — **succeeds** |
| `std::stod("")` / `"abc"` | throws `std::invalid_argument` |
| `std::stod("1e400")` | throws `std::out_of_range` |

In Project 2 you tightened `stoi` by hand-validating every character. There is a cleaner
tool for `stod`, because the grammar of a floating-point literal is too fiddly to
hand-validate (sign, digits, point, exponent, exponent sign, hex floats…):

```cpp
std::size_t consumed { 0 };
const double v { std::stod(token, &consumed) };
// consumed == number of characters stod actually used
```

If `consumed != token.size()`, there was trailing garbage. That plus `std::isfinite` plus
catching `out_of_range` is a complete validator, and it is *shorter* than the Project 2
hand-rolled one. `std::stoi` has the same parameter — useful for that Project 2 stretch
goal you left open.

### Aside: `std::from_chars`, the tool you'll want on a microcontroller

`std::stod` has two properties that are fine here and disqualifying in embedded work:

1. **It throws.** Embedded builds routinely use `-fno-exceptions`, which makes `stod`
   unusable outright.
2. **It is locale-dependent.** In a locale where the decimal separator is `,`, `stod("3.14")`
   returns `3.0` and `stod("3,14")` returns `3.14`. Your program silently produces wrong
   numbers on someone else's machine. This is a genuine, famous source of data-corruption
   bugs.

`std::from_chars` from `<charconv>` fixes both — no exceptions, no locale, no allocation:

```cpp
double value {};
const auto [ptr, ec] { std::from_chars(token.data(), token.data() + token.size(), value) };
// ec == std::errc{}            -> success
// ec == std::errc::result_out_of_range -> too big
// ptr == token.data() + token.size()   -> the whole token was consumed
```

(That `auto [ptr, ec]` is a **structured binding** — C++17's way of unpacking a struct into
named variables, the same idea as Python's `a, b = pair`.)

For this project **use `std::stod`**, because it keeps continuity with the exception
handling you learned in Project 2 and it is genuinely simpler. Switching to `from_chars` is
stretch goal 9, and it is the version you would actually ship.

### Other `<cmath>` you need

`std::sqrt`, `std::fabs` (**not** `std::abs`, which has integer overloads that will
silently truncate), `std::isfinite`, `std::isnan`.

---

## 3. File I/O — `<fstream>`

### Reading

```cpp
#include <fstream>

std::ifstream file { "data.csv" };     // opens on construction
if (!file) {
    return std::unexpected(std::format("could not open '{}'", path));
}

std::string line {};
while (std::getline(file, line)) {
    // ...
}
```

Three things to notice:

**1. This is your `with` block.** You asked what plays the role of Python's
`with open(...) as f:`. The answer is **scope**. `std::ifstream` is an RAII type: its
constructor opens the file, its destructor closes it, and the destructor runs automatically
when `file` goes out of scope — on normal exit, on an early `return`, and during exception
unwinding. There is no `close()` to remember and no `finally` to write. Same mechanism from
Project 2 that makes a `std::vector` free itself. `with` is Python's bolt-on for a problem
C++ solved in the type system.

**2. `if (!file)` is the Project 2 `bool` conversion again.** A stream converts to `bool`
via its state flags — false if `failbit` or `badbit` is set. A nonexistent file, a
permissions error, or a directory instead of a file all set `failbit` at construction.
**Check before reading.** Skip the check and `getline` immediately returns false, the loop
body never runs, and you get a clean, silent, empty result — indistinguishable from an
empty file.

**3. `std::getline(file, line)` is the identical call you used with `std::cin`.** Not a
coincidence: `std::ifstream` and `std::cin` both derive from `std::istream`, and `getline`
takes `std::istream&`. Anything written against `std::istream&` works with a file, the
keyboard, or a string buffer, unchanged. Same on the output side with `std::ostream&`:
`std::cout`, an `std::ofstream`, and `std::ostringstream` are interchangeable. **You will
use this deliberately in this project** — one report function, two destinations.

That is your first real taste of runtime polymorphism, and it is worth sitting with: the
function does not know or care what kind of stream it got.

### Writing

```cpp
std::ofstream out { "report.txt" };                // creates/TRUNCATES
std::ofstream app { "log.txt", std::ios::app };    // appends
if (!out) { /* handle */ }
out << std::format("count: {}\n", n);
```

`ofstream` **truncates by default** — opening an existing file destroys its contents
immediately, before you write anything. Be sure about the filename.

### Distinguishing "reached the end" from "the read broke"

```cpp
if (file.bad()) { /* hardware/IO error — the data is suspect */ }
```

`eof()` is the normal, expected termination. `bad()` means something actually went wrong.
Optional polish here, but know the difference exists.

### Two practical traps for your setup specifically

**Relative paths resolve against the *working directory*, not the source file's directory.**
If you run `./imu_analyzer` from the project folder it finds `imu_log.csv`; if VS Code's
debugger launches with a different `cwd`, it will not, and you will get "could not open" on
a file you can plainly see. When that happens, print the path you tried and run `pwd`.

`<filesystem>` (C++17) is the modern way to be precise about this:
`std::filesystem::current_path()`, `std::filesystem::absolute(p)`,
`std::filesystem::exists(p)`. Including the absolute path in your error message costs one
line and saves an hour. Optional, recommended.

**Windows line endings.** Your project lives on a OneDrive-synced Windows path
(`/mnt/c/Users/mark/OneDrive - Auburn University/...`) read from WSL. A file authored or
touched by a Windows tool has `\r\n` line endings; `std::getline` strips the `\n` and
**leaves the `\r`** as the last character. Your Project 2 `trim` uses `std::isspace`, which
includes `\r`, so it already handles this — that "widening of the contract" you documented
turns out to have been the right call. The fixture has one deliberate `\r` line to prove it.

(Second-order note on that path: it has **spaces in it**, and it is on a DrvFs mount. Quote
your paths in shell commands, and expect builds to be noticeably slower than they would be
under `~/`. If compile times start annoying you, that's why.)

---

## 4. Iterators and half-open ranges

`v.begin()` is a position pointing at the first element. `v.end()` points **one past the
last element** — it is *not* the last element, it is not inclusive, and **dereferencing it
is undefined behavior**.

```
   v = [ 10, 20, 30 ]
         ^          ^
      begin()     end()
```

You already use this convention without thinking about it: Python's `range(0, n)` stops at
`n-1`, and `s[0:n]` excludes index `n`. Same idea. The payoff of half-open ranges:

- `end - begin == size`, with no off-by-one
- an empty range is exactly `begin == end` — no special case
- `[a, b)` and `[b, c)` splice into `[a, c)` cleanly

Nearly every `<algorithm>` function takes a `begin, end` pair, which is why they work
identically on vectors, arrays, and strings — the algorithm never learns what container it
is in.

```cpp
for (auto it = v.begin(); it != v.end(); ++it) {
    std::cout << *it << '\n';   // * dereferences, like a pointer
}
```

Use `!=`, not `<` (only random-access iterators support `<`). And this is exactly what your
range-based `for` has been compiling into all along.

**`auto` earns its keep here.** The real type is
`std::vector<ImuReading>::const_iterator`; nobody writes that.

**Invalidation, carried from Project 2:** a `push_back` that reallocates invalidates every
iterator, pointer, and reference into the vector. Do not hold an iterator across a
`push_back`.

**C++20 note:** `<ranges>` lets you pass the container directly —
`std::ranges::sort(v, cmp)` instead of `std::sort(v.begin(), v.end(), cmp)`. That is
strictly nicer, and we will use it *after* you have written the iterator form at least
once, because half the standard library and effectively all of the error messages still
speak iterators.

---

## 5. `std::sort` and the comparator contract

```cpp
#include <algorithm>

std::vector<int> v { 5, 2, 9, 1 };
std::sort(v.begin(), v.end());              // ascending, uses operator<
std::sort(v.begin(), v.end(), comparator);  // your ordering
```

`std::sort` is O(n log n) (introsort: quicksort, falling back to heapsort, with insertion
sort for small ranges). It is **not stable** — equivalent elements may be reordered.
`std::stable_sort` preserves their relative order and costs a bit more.

Your struct has no `operator<`, so the two-argument form will not compile for it. You must
supply a comparator.

### The contract

`comp(a, b)` must return `true` **if and only if `a` must come strictly before `b`**.

This must be a **strict weak ordering**:

| Rule | Meaning |
|---|---|
| Irreflexive | `comp(a, a)` is **false**, always |
| Asymmetric | if `comp(a, b)` then `!comp(b, a)` |
| Transitive | `comp(a,b) && comp(b,c)` ⇒ `comp(a,c)` |
| Transitive equivalence | if `a` ≡ `b` and `b` ≡ `c` (neither ordered) then `a` ≡ `c` |

### The `<=` bug

```cpp
// WRONG - the most common std::sort bug in existence
std::sort(v.begin(), v.end(), [](int a, int b) { return a <= b; });
```

It looks harmless and may even produce sorted output on your test data. But `comp(a, a)`
returns `true`, violating irreflexivity, and `std::sort`'s inner loop uses the comparator
itself as its loop guard. With a comparator that never reports "equal," that guard **runs
off the end of the array and writes outside the buffer**. Not a wrong answer — a
memory-corrupting crash, later, in unrelated code.

**Rule: comparators use `<` or `>`, never `<=` or `>=`.**

For descending order, do not negate — flip the operands: `return b.mag < a.mag;`

### Multi-key comparison

```cpp
if (a.key1 != b.key1) { return a.key1 < b.key1; }
return a.key2 < b.key2;
```

Never `return a.key1 < b.key1 || a.key2 < b.key2;` — that is not a valid ordering.

### `<=>`, the three-way comparison operator (C++20)

```cpp
struct Version {
    int major {};
    int minor {};
    auto operator<=>(const Version&) const = default;
};
```

That single defaulted line generates `<`, `>`, `<=`, `>=`, `==`, and `!=`, comparing
members **lexicographically in declaration order** — major first, minor as the tiebreak.
It replaces the six hand-written operators that C++17 required, and it cannot get them
mutually inconsistent.

`a <=> b` itself returns an *ordering category*, not a bool:

| Category | Meaning | You get it when |
|---|---|---|
| `std::strong_ordering` | exactly one of `<`, `==`, `>` | integers |
| `std::weak_ordering` | as above, but "equal" means "equivalent, not identical" | case-insensitive strings |
| `std::partial_ordering` | some pairs are **unordered** | **anything containing `double`** |

That last row is §2's NaN lesson, encoded in the type system. Default `<=>` on a struct of
doubles and the compiler hands you `partial_ordering`, because `NaN <=> 1.0` is genuinely
`unordered` — not less, not greater, not equal. And **`std::sort` will not accept a
partial ordering**, which is the language telling you to eliminate NaN before sorting
rather than hope.

For this project, write the comparators by hand as lambdas — the contract is the lesson.
Defaulted `<=>` is worth knowing about and worth trying on the struct just to see which
category you get.

### `std::ranges::sort` and projections (C++20)

```cpp
std::ranges::sort(v, cmp);                        // no .begin()/.end()
std::ranges::sort(v, {}, &Sample::y);             // sort by a member, no lambda at all
std::ranges::sort(v, std::greater {}, &Sample::y); // descending by that member
```

That third argument is a **projection**: "before comparing, apply this to each element."
It is `sorted(key=...)` from Python, and `&Sample::y` is a pointer-to-member, a thing you
have not met yet but which reads exactly as it looks.

Ranges algorithms are also **safer**: they take one container, so you cannot accidentally
pass `v1.begin(), v2.end()` — a mistake `std::sort` will happily accept and then corrupt
memory over.

Requirement for this project: **write the `std::sort` + lambda version.** Then, as a
one-line stretch, express one of the three sorts as a ranges call with a projection and see
how much disappears.

### Other `<algorithm>` worth knowing

`std::max_element` / `std::min_element` (return iterators), `std::adjacent_find`,
`std::count_if`, `std::nth_element` (partial sort — the efficient median),
`std::is_sorted` (excellent as an assertion in a test).

---

## 6. Lambdas

You know these from Python, so this is mostly syntax plus one genuinely new idea.

```cpp
[captures](parameters) -> return_type { body }
```

The return type is almost always deducible; omit `-> T`.

```python
key = lambda p: p.y          # Python
```
```cpp
auto cmp = [](const Point& a, const Point& b) { return a.y < b.y; };   // C++
```

### The new idea: captures are explicit

A Python lambda reaches out and grabs enclosing variables by name automatically. C++
requires you to say what it may see, because C++ must decide **copy or reference** and that
decision has lifetime consequences.

| Capture | Meaning |
|---|---|
| `[]` | captures nothing — only its parameters |
| `[x]` | `x` **by value** (copied when the lambda is created) |
| `[&x]` | `x` **by reference** (alias to the original) |
| `[=]` | everything used, by value |
| `[&]` | everything used, by reference |

```cpp
const double threshold { 4.0 };
auto is_big = [threshold](const Point& p) { return p.y > threshold; };
```

**The trap:** a lambda that captures by reference and outlives what it captured holds a
dangling reference — UB. If the lambda is used and discarded in the same scope (a
`std::sort` comparator, always), `[&]` is safe. If it is stored, returned, or run later,
capture by value. Prefer naming exactly what you capture over blanket `[=]` / `[&]`; it
makes the lifetime question answerable by reading one line.

### Two facts worth carrying

**Every lambda has its own unique, unnameable, compiler-generated type.** That is why you
store one in `auto` — you cannot write its type. It also means passing a lambda to
`std::sort` lets the compiler **inline the comparison**, whereas a function *pointer*
usually cannot be inlined. A lambda comparator is typically measurably faster than the
equivalent free function passed by pointer. Unusual: the convenient thing is also the
faster thing.

**A lambda with an empty capture list `[]` converts implicitly to a plain C function
pointer.** One with captures does not. That is the whole story of why interrupt handlers
and C callbacks on microcontrollers take capture-less lambdas and reject stateful ones —
there is nowhere in a raw function pointer to put the captured state. You will hit this the
first time you register an ISR.

(`mutable` after the parameter list allows modifying by-value captures. Not needed here.)

---

## 7. `enum class`

You said you have not used Java or Python enums, so here is the whole thing, starting with
why the old version was broken.

### The C-style enum and its two failures

```cpp
enum Fruit { Apple, Banana, Cherry };   // Apple==0, Banana==1, Cherry==2
enum Pet   { Cat, Dog };                // Cat==0, Dog==1
```

**Failure 1 — the names leak into the enclosing scope.** `Apple` is a global name. Declare
another enum anywhere with a `Cherry` and you have a redefinition error. This is why C code
is full of `COLOR_RED`, `FRUIT_APPLE` — manual namespacing to work around it.

**Failure 2 — they implicitly convert to `int`.**

```cpp
if (Apple == Cat) { ... }   // compiles, and is TRUE (0 == 0)
int x = Banana + 7;         // compiles, x == 8
Fruit f = Fruit(99);        // compiles, f is not any fruit
```

There is no fruit that is a cat, but the compiler cannot tell, because as far as it is
concerned you compared two ints. An `enum` was never really a type — it was named integer
constants wearing a type's clothes.

### `enum class` (C++11) fixes both

```cpp
enum class Fruit { Apple, Banana, Cherry };
enum class Pet   { Cat, Dog };

Fruit f { Fruit::Apple };       // must qualify — names are scoped
if (f == Pet::Cat)  { }         // COMPILE ERROR. Good.
int x { Fruit::Banana };        // COMPILE ERROR. Good.
if (f == Fruit::Apple) { }      // fine — same type
```

To get the numeric value, C++23 gives you a named tool instead of a cast:

```cpp
#include <utility>
const auto n { std::to_underlying(Fruit::Banana) };   // 1, correctly typed
```

Prefer it over `static_cast<int>`. A cast is a *claim* (Project 1's theme); `to_underlying`
is a *fact* — it always produces the enum's actual underlying type, and it keeps working if
you later pin that type to something other than `int`.

Comparison **within** the same enum works, including `<` and `>`, comparing the underlying
values — so **the order in which you declare enumerators defines an ordering you can sort
by.** Declare them meaningfully and it comes free.

### The `switch` trick — correct by construction

```cpp
std::string to_string(Fruit f) {
    switch (f) {
        case Fruit::Apple:  return "apple";
        case Fruit::Banana: return "banana";
        case Fruit::Cherry: return "cherry";
    }
    return "unknown";   // unreachable for valid input; silences -Wreturn-type
}
```

**Deliberately omit `default:`.** With `default:` present, the compiler assumes you handled
everything. Without it, `-Wswitch` (part of `-Wall`) **warns about every enumerator you did
not handle**. The day you add a fourth fruit, the compiler tells you which switches need
updating instead of you finding out at runtime.

"Correct by construction over correct by maintenance," cashed out as a compiler flag. Do it
this way in this project.

### No `operator<<`, and no `std::format` either

`std::cout << Fruit::Apple` does not compile — no stream overload for your enum, and no
implicit conversion to rescue it. `std::format("{}", Fruit::Apple)` also does not compile,
for the same reason. You write a `to_string` and print that. This is a feature: printing an
enum as a bare integer is almost never what you wanted.

### Choosing the underlying type

```cpp
enum class Register : std::uint8_t { Ctrl = 0x6B, Accel = 0x3B };   // <cstdint>
```

By default an `enum class` is backed by `int` (4 bytes). Pinning it to `std::uint8_t` makes
it one byte and lets you name real register addresses at their real width. Directly
relevant to the MPU-6050 you are pretending to read from.

---

## 8. `std::optional` and `std::expected`

Two return channels for "this might not produce a value." Choosing between them is a real
design decision, and this project uses **both, deliberately**.

### `std::optional<T>` (C++17) — a value, or nothing

`Optional[T]` / `None` from Python, except the compiler enforces that you check.

```cpp
#include <optional>

std::optional<double> half_of(double x) {
    if (x < 0.0) { return std::nullopt; }   // "nothing"
    return x / 2.0;                          // implicitly wraps
}

const std::optional<double> r { half_of(10.0) };
if (r) {                       // or r.has_value()
    std::cout << *r << '\n';   // * unwraps
}
const double d { r.value_or(0.0) };
```

| Access | On empty |
|---|---|
| `*opt`, `opt->member` | **UB.** No check. |
| `opt.value()` | throws `std::bad_optional_access` |
| `opt.value_or(fallback)` | returns the fallback |

Exactly the `v[i]` / `v.at(i)` split from Project 2, same reasoning: unchecked when you
have already proven it is occupied, checked when you have not.

### `std::expected<T, E>` (C++23) — a value, or a reason

```cpp
#include <expected>

std::expected<double, std::string> reciprocal(double v) {
    if (v == 0.0) { return std::unexpected("division by zero"); }
    return 1.0 / v;
}

const auto r { reciprocal(0.0) };
if (r) { std::cout << *r; }
else   { std::cout << r.error(); }   // .error() gets the E
```

Same shape as `optional` — `if (r)`, `*r`, `.value()` (throws `std::bad_expected_access`) —
plus `.error()`. `E` can be anything: a string, an `enum class`, a struct with a line
number.

This is Rust's `Result<T, E>` arriving in C++, and it is the mechanism the embedded world
converged on, because it gives you exception-quality error information **with no
exceptions** — no unwinding, no runtime overhead, works under `-fno-exceptions`.

### Monadic operations (C++23, on both)

```cpp
const auto out { parse(text)
                    .transform([](double v) { return v * 2.0; })   // map the value
                    .and_then(validate)                            // chain another fallible step
                    .or_else(use_default) };                       // recover
```

Each step short-circuits if the previous one failed. Python's closest equivalent is chained
`if x is not None` checks — this is that, without the ladder. Not required here; worth
knowing it exists, and there is one place in the fixture pipeline where `.transform()` fits
naturally if you want to try it.

### Choosing between them, and versus your Project 2 pattern

Your Project 2 `parse_int(const std::string&, int&, std::string&)` was option 2 from the
error menu — `bool` plus out-parameters. Compare all three honestly:

| | `bool` + out-params | `optional<T>` | `expected<T,E>` |
|---|---|---|---|
| Caller needs a pre-declared variable | yes — Round 1 finding 4 was a bug caused by exactly that | no | no |
| Can the caller silently ignore failure? | yes | not without visibly unwrapping | not without visibly unwrapping |
| Composes as a return value | no | yes | yes |
| Carries *why* it failed | yes | **no** | yes |
| Works with `-fno-exceptions` | yes | yes | yes |

`std::expected` dominates the out-param version outright. `optional` versus `expected` is a
genuine choice, and the question is only: **does the caller need to know why?**

This project answers it differently in two places, thirty lines apart:

- **`parse_double`** → `std::optional<double>`. The caller already knows the line number
  and which column it was on, and can write a better message than the parser could. A
  reason channel would be redundant noise.
- **`parse_row`** → `std::expected<ImuReading, std::string>`. Here the reason genuinely
  varies — wrong field count, unparseable field, non-finite value — and the caller cannot
  reconstruct it.

Be ready to defend each choice on its own terms at review. "I used what the spec said" is
not a defense.

---

## 9. `operator<<`, and the answer I owed you

You know `__str__`/`__repr__`, so the concept transfers: define how your type prints. The
structure differs in three ways.

### What operator overloading actually is

An operator is a function with unusual call syntax. `a + b` is `operator+(a, b)`; `a << b`
is `operator<<(a, b)`. Overloading one means defining that function for your own types.
There is no magic — no dispatch table, no dunder lookup. It is name resolution.

### The owed explanation: `<<` and `>>`

`<<` and `>>` are the **bit-shift operators**. `1 << 3` is `8`; `x >> 1` halves an unsigned
int. That is their original and still-primary meaning, and you will use it constantly on
microcontrollers (`PORTB |= (1 << 5)`).

In 1985 the iostream library **overloaded** them to mean "insert into a stream" and
"extract from a stream," chosen essentially because the arrows point in the direction the
data flows and the operators were available. There is no conceptual relationship to
shifting whatsoever.

Why people object:

1. **It is a pun.** `std::cout << x` says "left-shift cout by x." The meaning is pure
   convention.
2. **The precedence is inherited from shift, and it is wrong for the new job.** Shift binds
   tighter than comparison and looser than arithmetic:
   ```cpp
   std::cout << a + b;        // fine: (a+b) — arithmetic binds tighter
   std::cout << a == b;       // parses as (std::cout << a) == b  -> compile error
   std::cout << x & 1;        // parses as (std::cout << x) & 1   -> nonsense
   std::cout << (x & 1);      // what you meant
   ```
   Nobody chose that precedence for streams; it came with the symbol.
3. **It made formatting verbose.** `printf("%5.2f\n", x)` versus
   `std::cout << std::fixed << std::setprecision(2) << std::setw(5) << x << '\n'`.

That third objection is the one the language finally acted on: **C++20 added `std::format`
and C++23 added `std::print`** (§10), which is why you are learning `operator<<` for the
*structure* it teaches and `std::format` for the *formatting*.

### Why it must be a free function

```cpp
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << std::format("({:.3f}, {:.3f})", p.x, p.y);
}
```

For `a @ b`, a **member** operator belongs to the type of `a` — the left operand. In
`std::cout << p`, the left operand is `std::ostream`, and you cannot add members to
`std::ostream`. So it must be a **free function** taking the stream first. (A member version
would only enable `p << std::cout`, which is backwards.)

That is a structural difference from `__str__`, which lives on the object.

### Why it returns `std::ostream&`

So it chains. `os << a << b` parses as `(os << a) << b` — the first call's *return value* is
the left operand of the second. Return the stream by reference and chaining works; return
`void` and `std::cout << p << '\n'` fails to compile.

### And why it takes `std::ostream&`, not `std::cout`

Because of §3: the same overload then serves `std::cout`, your `ofstream`, and a string
stream. **Write your report function as `void write_report(std::ostream& os, ...)` and call
it twice — once with `std::cout`, once with an `ofstream`.** One implementation, two
destinations, zero duplication. That is the payoff, and it is a requirement.

### The modern alternative: `std::formatter`

To make your type work with `std::format("{}", reading)` instead of `os << reading`, you
specialize `std::formatter<ImuReading>`. It is more code than `operator<<` and involves
template specialization you have not learned, but it composes with the whole formatting
system — width, alignment, and nested format specs all work.

For this project: **`operator<<` is required.** `std::formatter` is stretch goal 10.
Knowing both exist, and that the ecosystem is mid-migration between them, is the point.

---

## 10. `std::format` and `std::print` — modern output

This is the section that replaces `<iomanip>`, and it should feel immediately familiar:
`std::format` is Python's `str.format` with essentially the same mini-language.

```cpp
#include <format>   // std::format  -> returns a std::string
#include <print>    // std::print / std::println -> writes directly

std::println("count: {}", n);                        // to stdout, adds '\n'
std::print("{:>10.4f}\n", value);                    // to stdout, no newline
std::print(os, "{:>10.4f}\n", value);                // to any std::ostream&
os << std::format("{:>10.4f}\n", value);             // works everywhere, any GCC 13+
```

Format spec, abbreviated — `{:[fill][align][sign][#][0][width][.precision][type]}`:

| Spec | Result for `3.14159` / `255` |
|---|---|
| `{:.3f}` | `3.142` — fixed, 3 decimals |
| `{:10.3f}` | `     3.142` — width 10, right-aligned (numbers default right) |
| `{:<10.3f}` | `3.142     ` — left |
| `{:^10.3f}` | `  3.142   ` — centered |
| `{:+.2f}` | `+3.14` — always show sign |
| `{:e}` | `3.141590e+00` |
| `{:#06x}` | `0x00ff` — hex, `#` prefix, zero-padded to 6 |
| `{:b}` | `11111111` — binary, and useful to you on microcontrollers |
| `{}` for a `bool` | `true` / `false`, not `1` / `0` |

`{{` and `}}` are literal braces, same as Python.

### Why this is better than `<iomanip>`, concretely

**It is not sticky.** `std::fixed` and `std::setprecision` **change the stream permanently**
until you change them back — set precision once for a table and every unrelated number
printed later is affected, including from other functions. A `std::format` spec applies to
exactly one value and nothing else. That entire class of action-at-a-distance bug is gone.

**Errors are caught at compile time.** The format string is checked against the argument
types during compilation. `std::format("{:d}", 3.14)` will not compile. `printf("%d", 3.14)`
compiles and prints garbage.

**It is type-safe and extensible**, and it is roughly as fast as `printf` and considerably
faster than iostream chains.

`<iomanip>` still exists and you will see it in every pre-2020 codebase, so recognize
`std::setw` / `std::setprecision` when you read them. Do not write new code with them.

For this project, **use `std::format` or `std::print`.** `<iomanip>` is not banned outright
— if you want to try both and compare, that's a legitimate exercise — but the report should
end up in the modern style.

---

# Part 2 — Abstract

Build a command-line analyzer for a mock IMU telemetry log.

The program reads a text log of timestamped accelerometer and gyroscope readings from a
file, parses and validates every row, rejects the bad ones with a diagnosis and a line
number while keeping the good ones, and classifies each surviving reading against the
sensor's physical limits — normal, motion spike, rail-saturated, or a dropped I²C read. It
then reports on the batch: how many rows survived, counts by classification, magnitude
statistics, the actual achieved sample rate, and where samples went missing. Finally it
offers an interactive loop to re-sort the readings by different keys and to write the whole
report to a file.

The intellectual content is in three places. **Validation:** the log is a hostile artifact —
a real one always is — and the parser must survive it without producing a single wrong
number. **Ordering:** `std::sort` is a sharp tool that punishes an incorrect comparator with
memory corruption rather than a wrong answer. **Stream abstraction:** the report is written
once and emitted twice, to the console and to a file, because both are `std::ostream&`.

You reuse `split` and `trim` from Project 2 unchanged. You do not rewrite them.

---

# Part 3 — Specification

## Input file format

A UTF-8 text file, one record per line.

- A line whose **first non-whitespace character is `#`** is a comment → skip silently
- A line that is empty or all whitespace → skip silently
- Any other line is a data row and must contain **exactly 7 comma-separated fields**:

  `timestamp_s, accel_x_g, accel_y_g, accel_z_g, gyro_x_dps, gyro_y_dps, gyro_z_dps`

- Fields may have arbitrary surrounding whitespace, which is not significant
- Every field must parse as a **finite** `double` — the entire field consumed, no trailing
  characters, and `NaN` / `±inf` rejected
- Timestamps may arrive **out of order**; that is not an error
- Duplicate timestamps are **reported, not rejected**

The provided `imu_log.csv` exercises every one of these, including one line with a Windows
`\r` ending and one indented comment.

## Required declarations (exact)

```cpp
struct ImuReading {
    double timestamp_s {};
    double accel_x_g {};
    double accel_y_g {};
    double accel_z_g {};
    double gyro_x_dps {};
    double gyro_y_dps {};
    double gyro_z_dps {};
};

struct LoadResult {
    std::vector<ImuReading>  readings {};
    std::vector<std::string> errors {};
    std::size_t              rows_seen {};   // data rows, excluding comments and blanks
};

enum class Quality   { Ok, Spike, Saturated, Dropout };   // ascending severity
enum class SortField { Timestamp, Magnitude, Quality };

// --- reused verbatim from Project 2 ---
std::vector<std::string> split(const std::string& text, char delimiter);
std::string trim(const std::string& text);

// --- parsing ---
std::optional<double> parse_double(const std::string& token);
std::expected<ImuReading, std::string> parse_row(const std::string& line);
std::expected<LoadResult, std::string> load_log(const std::string& path);

// --- analysis ---
double  accel_magnitude_g(const ImuReading& r);
Quality classify(const ImuReading& r);
std::string to_string(Quality q);
std::string to_string(SortField f);

void sort_readings(std::vector<ImuReading>& readings, SortField field);

// precondition: readings sorted ascending by timestamp
std::size_t count_duplicate_timestamps(const std::vector<ImuReading>& readings);

// --- output ---
std::ostream& operator<<(std::ostream& os, const ImuReading& r);
void write_report(std::ostream& os, const std::string& source_path, const LoadResult& result);
```

Notes, since as in Projects 1 and 2 several signatures encode decisions:

- **`parse_double` returns `optional`; `parse_row` returns `expected`.** Deliberate contrast
  — see §8. Be ready to defend both at review.
- **`load_log` returns `expected<LoadResult, std::string>`.** The `expected` error channel is
  reserved for the one genuinely fatal condition: *the file could not be opened*. Bad **rows**
  are not failures of `load_log` — they go into `LoadResult::errors` and the function still
  succeeds. Getting this distinction right is most of the design work in this function.
- **`LoadResult` replaces three out-parameters.** Returning a struct by value is the correct
  default: NRVO constructs it in place and the vectors move rather than copy. This is the
  Project 2 lesson ("do not contort a signature into an out-parameter for performance")
  applied to a function with three things to return.
- **`write_report` takes `std::ostream&`, not `std::ofstream&`.** Non-negotiable; the whole
  point is that `std::cout` and a file both satisfy it.
- **`to_string` is overloaded** on two enum types. It is safe to leave unqualified even
  though `std::to_string` exists, because your enums live in the global namespace, so ADL
  finds yours. Contrast with the Project 2 decision to rename `min`/`max` — there the
  arguments were `std::vector`, which *is* in `std`, which is what made ADL dangerous. Same
  mechanism, opposite conclusion, because the argument types differ.
- **`count_duplicate_timestamps` carries a precondition.** Document it, and make violating it
  structurally impossible at the call site, exactly as you did for `mean`/`min_value` in
  Project 2.

## Sensor limits (name these; no magic numbers)

| Constant | Value | Meaning |
|---|---|---|
| accel full scale | `16.0` g | MPU-6050 max range |
| gyro full scale | `2000.0` dps | MPU-6050 max range |
| spike threshold | `4.0` g | accel magnitude above this is a motion spike |
| expected fields | `7` | |

## Classification — priority order, first match wins

1. **`Dropout`** — all six sensor components are *exactly* `0.0` (timestamp exempt). A failed
   I²C read returns a zeroed buffer; physically impossible, since gravity is always present.
   Exact `== 0.0` is correct here — see §2.
2. **`Saturated`** — `|any accel component| >= 16.0` **or** `|any gyro component| >= 2000.0`.
   The sensor is railed; the true value is unknown and at least this large.
3. **`Spike`** — accel magnitude `> 4.0` g.
4. **`Ok`** — everything else.

The order matters and must be documented, because a railed reading also exceeds the spike
threshold. The enumerator declaration order runs Ok → Spike → Saturated → Dropout, i.e.
ascending severity, which is what makes "sort worst-first" a one-line comparator.

## Sorting

`sort_readings` must use `std::sort` with a **lambda** comparator.

| `SortField` | Order |
|---|---|
| `Timestamp` | ascending |
| `Magnitude` | **descending** (most interesting first) |
| `Quality` | **worst first** (Dropout → Saturated → Spike → Ok) |

**Output must be deterministic**: two readings that tie on the primary key must always come
out in the same relative order. Achieve that either by tie-breaking on timestamp inside the
comparator, or by `std::stable_sort` over an already-timestamp-sorted vector. Pick one, and
be ready to say why.

## Report contents

`write_report` emits, in a readable layout formatted with `std::format` / `std::print`:

- source file path
- rows seen, rows accepted, rows rejected
- every rejection, as `line N: <reason>`
- timestamp range and duration
- **estimated sample rate** in Hz, from the mean interval between consecutive timestamps
- **largest gap** between consecutive timestamps, and the timestamp it starts at
- duplicate timestamp count
- **count of each `Quality`**, using the `to_string` overload
- min / max / mean accelerometer magnitude, and the timestamp at which the max occurred
- the first `report_sample_count` (10) readings in the current sort order, via `operator<<`

Timestamps to 3 decimals, accelerations to 4, is a reasonable choice.

## Program flow

1. Load `imu_log.csv` from the working directory (hardcoded relative path — `argv` is
   stretch goal 4).
2. If `load_log` returns an error: print `.error()`, `return 1`.
3. If zero rows were accepted: print the rejection list and exit — nothing to analyze. Do
   not enter the menu.
4. Sort by timestamp immediately after loading, so every downstream precondition (duplicate
   detection, gap analysis, rate estimation) is satisfied by construction.
5. Print the summary report to `std::cout`.
6. Enter a prompt loop, using your Project 2 input machinery:

   ```
   [t] sort by timestamp        [m] sort by magnitude
   [q] sort by quality          [s] print summary again
   [w] write full report to imu_report.txt
   [quit]
   ```

   - `t` / `m` / `q` → re-sort, print the top 10 via `operator<<`
   - `s` → re-print the summary to `std::cout`
   - `w` → open an `std::ofstream` and call the **same** `write_report`; confirm, or report
     the failure if the file could not be opened
   - unrecognized input → say so, re-prompt
   - `quit` **or EOF (Ctrl+D)** → exit 0

   Project 2 finding 1 lives here: Ctrl+D must exit, not spin.

## Rules

**Allowed headers:** `<iostream>`, `<string>`, `<vector>`, `<optional>`, `<expected>`,
`<fstream>`, `<algorithm>`, `<ranges>`, `<format>`, `<print>`, `<cmath>`, `<cctype>`,
`<limits>`, `<stdexcept>`, `<utility>`, `<cstddef>`, `<cstdint>`, `<filesystem>`,
`<charconv>` (stretch only)

**Banned:**
- `<sstream>` — still. Use your `split`. (Third project running; it arrives in Project 4 and
  you should have earned the right to be annoyed by then.)
- `<map>` / `<unordered_map>` — count the qualities with a small fixed array or a `switch`.
  Hash tables are the wrong reach for four known categories, and are unavailable to you on
  most microcontrollers anyway.
- Hand-rolled sorting. Use `std::sort`; that is the point.
- `using namespace std;`

**Required:**
- **Designated initializers** wherever you construct an `ImuReading`
- At least one lambda, as a `std::sort` comparator
- `write_report` written once against `std::ostream&`, invoked with both `std::cout` and an
  `std::ofstream`
- Output formatted with `std::format` / `std::print`, not `<iomanip>`
- `std::to_underlying` rather than `static_cast<int>` on any enum
- Every `switch` over an enum has **no `default:` label**, so `-Wswitch` covers you
- `split` and `trim` copied from Project 2 without modification
- No magic numbers; sensor limits and thresholds as `constexpr` at file scope
- Brace initialization throughout
- **`const` on locals by default.** Flagged explicitly: this was a finding in three
  consecutive review rounds in Project 2. Getting through Round 1 of this review with no
  missing `const` is a personal goal for this project.
- Correct value-vs-reference choice in every range-based `for` (same note)
- Documented preconditions, structurally guaranteed at the call site
- Clean build under the full strict flag set including `-Wconversion`
- **One run under sanitizers, reported.** Deferred in Projects 1 and 2; not deferrable now.
  `std::sort` with a bad comparator and doubles that might be NaN are precisely what UBSan
  and ASan exist to catch.

## Build

```bash
g++ -std=c++26 -Wall -Wextra -Wshadow -Wconversion -Wold-style-cast -pedantic -g \
    -o imu_analyzer imu_analyzer.cpp
```

New flag since Project 2: **`-Wold-style-cast`** warns on C-style casts `(int)x`, forcing
`static_cast` and friends. You already write `static_cast` by habit; this makes it
enforced rather than remembered.

Sanitizer run (required at least once):

```bash
g++ -std=c++26 -Wall -Wextra -g -fsanitize=address,undefined -o imu_analyzer_san imu_analyzer.cpp
./imu_analyzer_san
```

Update `tasks.json` and the C/C++ extension's IntelliSense `cppStandard` to `c++26` so the
editor and compiler agree — otherwise VS Code will red-squiggle `std::expected` and
`std::print` while the build succeeds.

---

# Part 4 — Test matrix

## `parse_double`

| Token | Expected |
|---|---|
| `3.14` | `3.14` |
| `-0.0043` | `-0.0043` |
| `1.2e-2` | `0.012` |
| `9.9871e-1` | `0.99871` |
| `0` / `0.0` | `0.0` |
| `+5` | `5.0` |
| `` (empty) | nothing |
| `abc` | nothing |
| `0.0121xyz` | **nothing** — trailing garbage; `stod` alone would return `0.0121` |
| `1.5.2` | nothing |
| `nan` / `NaN` | **nothing** — parses successfully, must be rejected |
| `inf` / `-inf` | **nothing** — same |
| `1e400` | nothing (`out_of_range`) |
| `1e-400` | your call — underflows to `0.0` and may or may not throw. Decide and document. |

## `parse_row`

| Line | Expected |
|---|---|
| `0.010,0.0133,0.0051,1.0124,0.0871,0.1382,0.0649` | ok |
| `  0.250 ,  0.0110 , -0.0050 ,  0.9990 ,  0.30 , -0.10 ,  0.02  ` | ok, whitespace stripped |
| tab-after-comma variant | ok |
| line ending in `\r` | ok — `trim` handles it |
| 6 fields | error: wrong field count |
| 8 fields | error: wrong field count |
| `0.190,,-0.0043,...` | error: empty field, named by position |
| `0.200,abc,...` | error: unparseable field, named by position |
| `0.220,nan,...` | error: non-finite |
| `0.240,1e400,...` | error: out of range |

Error strings should name **which field** failed, not just that one did — the Project 2
lesson about error channels being contracts. The `expected` error type is a `std::string`
precisely so it can say that.

## `classify`

| Reading | Expected |
|---|---|
| `0.000, -0.0031, 0.0061, 0.9973, ...` | `Ok` (magnitude ≈ 0.997) |
| `0.120, 2.5, -3.1, 2.0, 180, -95, 44` | `Spike` (magnitude ≈ 4.457) |
| `0.130, 16.0, -2.2, 3.4, 410, -220, 88` | `Saturated` — **not** `Spike`, despite magnitude ≈ 16.5 |
| `0.140, 1.8, -0.9, 1.1, 2000.0, -140, 30` | `Saturated` via **gyro**, magnitude only ≈ 2.29 |
| `0.150, 0,0,0,0,0,0` | `Dropout` — **not** `Ok`, despite magnitude 0 being below every threshold |

The last two catch a classifier written in the wrong order or one that only looks at the
accelerometer.

## Sorting

| Check | Expected |
|---|---|
| sort by timestamp, then again by timestamp | identical output (idempotent) |
| sort by timestamp on an already-sorted vector | unchanged |
| sort by magnitude on the fixture | `0.130` first (≈16.5), `0.150` last (0.0) |
| sort by quality on the fixture | `0.150` (Dropout) first, the 26 `Ok` readings last |
| tie on magnitude or quality | resolved by timestamp, identical across runs |
| empty vector | does not crash |
| single element | does not crash |

## End-to-end on `imu_log.csv`

| Quantity | Expected |
|---|---|
| data rows seen (non-comment, non-blank) | **39** |
| rows accepted | **31** |
| rows rejected | **8** |
| `Ok` count | **26** |
| `Spike` count | **2** (both at t = 0.120) |
| `Saturated` count | **2** (t = 0.130, t = 0.140) |
| `Dropout` count | **1** (t = 0.150) |
| duplicate timestamps | **1** (the pair at t = 0.120) |
| first / last timestamp | 0.000 / 0.450 |
| duration | 0.450 s |
| largest gap | **0.090 s**, starting at t = 0.330 |
| max accel magnitude | ≈ **16.504** g at t = 0.130 |
| min accel magnitude | **0.000** g at t = 0.150 (the dropout) |
| estimated sample rate | ≈ **66.7 Hz** — see below |

**On that sample rate.** The nominal rate is 100 Hz, and most of the file is at 100 Hz. The
mean-interval estimate reports ~67 Hz because 30 intervals share a total span of 0.450 s
that includes one 90 ms gap and one zero-length interval from the duplicate. The number is
not a bug — it is the mean interval, correctly computed, being a poor estimator in the
presence of gaps. Report it as specified, then consider what the *median* interval would
say (stretch goal 2). This distinction matters in real telemetry: mean rate tells you
throughput, median interval tells you the sampling clock.

Also worth noticing: the file contains a genuine out-of-order pair (t = 0.070 before
t = 0.060) and one row in scientific notation (t = 0.080). Neither is an error, and if
either shows up in your rejection list, your loader is wrong.

---

# Part 5 — Stretch goals

1. **`parse_int` without `stoi`** — carried from Project 2, still open. Character arithmetic
   (`'7' - '0'`), detecting overflow *before* it happens.
2. **Median interval and median magnitude** via `std::nth_element`. O(n) rather than
   O(n log n), and a materially better sample-rate estimate.
3. **Dropped-sample detection.** Flag every interval longer than 1.5× the median and
   estimate how many samples were lost.
4. **Command-line file path** via `int main(int argc, char* argv[])`, defaulting to
   `imu_log.csv`.
5. **Cap the error list** at the first 10 rejections on the console with "…and N more,"
   while the file report keeps all of them.
6. **`enum class Quality : std::uint8_t`** — print `sizeof(Quality)` before and after,
   confirm 4 → 1.
7. **A rolling mean** over a window of N readings. Where iterator arithmetic starts paying
   off.
8. **Require and validate a header line** — the first non-comment line must name the seven
   columns in order; a mismatch is a fatal load error (and a natural use of `expected`'s
   error channel).
9. **Replace `std::stod` with `std::from_chars`** (§2 aside). No exceptions, no locale
   dependence. The version you would actually ship, and the only version that works on a
   microcontroller with `-fno-exceptions`.
10. **Specialize `std::formatter<ImuReading>`** so `std::format("{}", reading)` works
    alongside `os << reading` (§9).
11. **Express one sort as `std::ranges::sort` with a projection** and compare (§5).
12. **Try `auto operator<=>(const ImuReading&) const = default;`** and report which ordering
    category you get and why (§5 / §2).

---

# Part 6 — Open items carried from Project 2

| Item | Status in Project 3 |
|---|---|
| `<<` / `>>` explanation owed | **Paid** — §9 |
| `enum class` named but not taught | **Taught** — §7 |
| `std::optional` named but not taught | **Taught** — §8, required in `parse_double` |
| Sanitizers introduced but never run | **Now required** |
| `std::istringstream` | Still deferred. Project 4. |
| `parse_int` without `stoi` | Still open — stretch goal 1 |
| Median | Still open — stretch goal 2 |
| `token.empty()` guard relying on `operator[]` at `size()` | Revisit when you copy `parse_int` forward |
| Classes, constructors, RAII of your own | Project 4 |
| Multi-file compilation: headers, include guards, ODR, linking | Project 4 — **and now also C++20 modules**, which GCC 15 supports well enough to demonstrate (`import std;` works even in C++20 mode). We will learn headers first, because that is what your class and every existing codebase use, then see what modules fix. |

---

# Part 7 — Workflow reminder

Unchanged from Projects 1 and 2:

1. You build it. I do not write solution code — hints are questions and trace-it-yourself
   exercises. When stuck, tell me what you tried and what happened.
2. Working script → full code review (good / bad / change).
3. You fix, resubmit, iterate. Per your Project 2 request, **rounds after the first come as
   short numbered lists** with deep explanation only where you ask for it.
4. When it is clean, I generate `project-03-imu-log-analyzer.md` for the Claude project.

Run `toolchain_probe.cpp` first. Then ask about anything in Part 1 before you start
writing. Structs, file streams, and the sort comparator contract are the three you have
never seen, and the comparator contract is the one that punishes a guess with memory
corruption instead of a wrong answer.
