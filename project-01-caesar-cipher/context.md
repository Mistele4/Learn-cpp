# Learn C++ — Project 1 Context: Caesar Cipher

**Status:** Complete
**Date:** August 2026
**Deliverable:** `caeser_cipher.cpp`
**Baseline for calibration:** `bin-hex_converter.cpp` (pre-existing work, written before this project)

---

## Purpose of this document

Context carryover for future threads in the `Learn C++` project. Records what was taught,
what was built, what was found in review, and what remains open.

---

## Learner profile (as of Project 1)

**Strong:**
- Python (data science, ML, CV/image processing, hardware control — motorized stages, motor control)
- MATLAB (PI controllers, ODE solvers written from scratch)

**Rusty / partial:**
- Java (2 courses, zyBooks-scale exercises only, no real projects — could pick back up quickly)
- R (statistics exposure)
- C (embedded exposure, little original code)

**Environment:** VS Code + g++, `tasks.json` build task configured.

**Confirmed known before Project 1** (from the bin-hex converter):
functions with params/returns, nested loops, conditionals, `std::string` ops
(`.length()`, `.find()`, indexing, `+=`), `std::getline`, brace initialization,
`static_cast` to silence signed/unsigned warnings.

**Confirmed gaps before Project 1:**
references, `const`, range-based for, `char` arithmetic, input validation,
`std::vector`, object copy semantics.

---

## Concepts taught in Project 1

### 1. `char` is an integer type
A `char` **is** a one-byte integer; ASCII interpretation happens only at print time.

- `'a' + 1` → `98` (an `int`), **not** `'b'` — small types promote to `int` before arithmetic
- `'7' - '0'` → `7` — digit char to numeric value
- `'Q' - 'A'` → `16` (an `int`) — **zero-based index**, not position count
- Round trip: `char` → `int` index → arithmetic → `int` → `static_cast<char>`
- `<cctype>`: `std::isalpha`, `std::isupper`, `std::islower`, `std::isdigit`

### 2. Pass by value vs. reference
- `std::string s` parameter → **full copy**; mutating it does **not** affect the caller
- Major divergence from Python, where names bind to objects and mutation is visible to the caller
- `std::string& s` → reference/alias; mutation **does** affect the caller

### 3. `const`
- Rough analogue of Java's `final`; compiler-enforced
- `const std::string&` = no copy (`&`) **and** no modification (`const`) — the idiomatic
  read-only parameter. Both halves matter; neither alone is sufficient.
- Parameter rule of thumb: read-only class type → `const T&`; must modify caller's → `T&`;
  small primitive → `T` by value
- Habit adopted: **locals `const` by default**, drop it only when mutation is needed
- `constexpr` > `const` for compile-time-known values (stronger guarantee, usable where
  compile-time values are required)

### 4. Signed vs. unsigned
- `.length()` returns `std::size_t` (unsigned) — cannot go negative, **wraps** instead
- `empty.length() - 1` → 18446744073709551615
- In mixed comparisons, the **signed** operand converts to unsigned (`-1 < str.length()` is `false`)
- Fixes: match the type (`std::size_t i`) or avoid indices (range-based for)

### 5. Range-based for loops
- `for (char c : str)` — copy
- `for (char& c : str)` — reference, mutates in place
- `for (const char& c : str)` / `const auto&` — read-only reference

### 6. `%` with negatives — C++ vs. Python
- Python: `-3 % 26` → `23`
- C++: `-3 % 26` → `-3` (result takes the sign of the **left** operand)
- `%` alone will **not** wrap a negative into a valid range

### 7. `>>` / `getline` buffer interaction
`std::cin` is a character queue, not an "ask the user" function.

| Operation | Skips leading whitespace? | Consumes trailing `\n`? |
|---|---|---|
| `>>` | Yes | **No** — leaves it in the buffer |
| `getline` | **No** | Yes — discards it |

A `getline` following a `>>` reads the leftover newline and returns an empty string —
it doesn't fail or get skipped, it correctly reads an empty line.

Fixes: `std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n')`, or
read everything with `getline` + `std::stoi` (the route taken here).

### 8. Scope and performance (raised by the learner)
- Locals live on the **stack**; space is reserved once at function entry, not per iteration
- A `char` local typically never reaches memory — it lives in a register
- Loop-scoped vs. hoisted primitive → near-certainly identical generated code
- Hoisting only helps for types that **own a resource** (`std::string`, `std::vector`, handles),
  where it enables capacity reuse
- Rule: **scope tightly by default; hoist only for resource-owning types, and only when measured**
- Optimizer behavior cannot be predicted by reading source — profile or write the clearest thing
- The hoisted version traded a zero performance gain for a real maintenance hazard

---

## Project specification (as issued)

Command-line Caesar cipher. Encrypt a message by rotating alphabetic characters,
decrypt it back, print all three strings to demonstrate the round trip.

**Required signatures (exact):**
```cpp
std::string encrypt(const std::string& text, int shift);
std::string decrypt(const std::string& text, int shift);
```

**Behavior:**
1. Letters shift; case is preserved
2. Wrapping (`z`+1 → `a`, `Y`+3 → `B`)
3. Non-alphabetic characters pass through untouched
4. **Any** integer shift must work — 0, 29, 100, −5, −1000
5. `decrypt(encrypt(text, s), s) == text` for all inputs

**Rules:**
- Standard library only (`<iostream>`, `<string>`, `<cctype>`, `<limits>`)
- **No arrays or `std::vector`** — no alphabet lookup table; character arithmetic only
- Clean build under `-Wall -Wextra` is part of the deliverable
- No magic numbers — `26` must be named
- Brace initialization, consistent with prior style

**Stretch:** implement `decrypt` in terms of `encrypt` (achieved); brute-force all 25
decryptions (not attempted)

---

## Test matrix

| Input | Shift | Expected |
|---|---|---|
| `abc` | 3 | `def` |
| `xyz` | 3 | `abc` |
| `Hello, World!` | 5 | `Mjqqt, Btwqi!` |
| `Hello` | 0 | `Hello` |
| `abc` | 29 | `def` |
| `def` | −3 | `abc` |
| `abc` | −29 | `xyz` |
| `` (empty) | 7 | `` (empty) |
| `12345 !?` | 10 | `12345 !?` |
| `abc` | 27 | `bcd` |
| `abc` | −1 | `zab` |
| `abc` | 100 | `wxy` |
| `abc` | −100 | `efg` |

> **Note:** the last row was originally issued as `ezf`, which was an error on the
> instructor's part. −100 mod 26 = 4, so `efg` is correct. Flagged here as a reminder to
> verify expected values rather than debug toward them.

---

## Review findings (all resolved)

| # | Finding | Resolution |
|---|---|---|
| 1 | **Out-of-range output for shifts ≥ 26 or negative.** `(c-'a'+shift) % 26` can yield a negative index, so `'a' + x` lands off the alphabet. Small negative shifts on later letters masked it. | Normalize `shift` into `[0, 25]` **before** the loop: `shift %= num_letters; if (shift < 0) shift += num_letters;` |
| 2 | **UB in `<cctype>` calls.** `char` is signed on g++/x86; these functions require a value representable as `unsigned char`. Any byte ≥ 128 (UTF-8, curly quotes, accents, em-dashes) is negative → out-of-bounds table read. | `static_cast<unsigned char>(c)` on every `<cctype>` argument |
| 3 | Magic number `26` appeared bare, twice | Named constant |
| 4 | **Implicit narrowing** on `encrypted += 'A' + (...)`; the RHS is an `int`. Silently launders out-of-range values into garbage characters instead of failing loudly. | Explicit `static_cast<char>` |
| 5 | Style: redundant `{ "" }` init, over-indented brace, duplicated branch arithmetic | Cleaned |
| 6 | **`-INT_MIN` is signed overflow (UB)** in `decrypt`. `int` range is asymmetric — one more negative value than positive. | Reduce before negating: `encrypt(text, -(shift % num_letters))`. Safe because the divisor is positive, so `INT_MIN % 26` is well-defined and the result is in `[-25, 25]`. |
| 7 | `base` hoisted outside the loop, carrying state across iterations; correctness depended on a manual `base = 'a';` reset that a future edit could silently break | Declared inside the loop via the conditional operator; renamed from `a` |
| 8 | `num_letters` defined twice, in two syntaxes | Single `constexpr` at file scope |
| 9 | Dead store — the `base = 'a';` reset became unobservable once scope tightened | Deleted |
| 10 | `base` assigned once but not marked `const` | `const char base { ... }` |

**Correctness argument for the final version:** after normalization, `shift ∈ [0, 25]`,
so `c - base + shift ∈ [0, 50]`, so `% 26 ∈ [0, 25]` — exactly the range `base + x` requires.
Normalizing *first* also prevents overflow in the addition for very large shifts.
Every path is provably in bounds for every `int` input.

---

## Build configuration

```bash
g++ -std=c++17 -Wall -Wextra -Wshadow -Wconversion -pedantic -g -o caesar caeser_cipher.cpp
```

| Flag | Catches |
|---|---|
| `-Wall` | Common real bugs (despite the name, **not** all warnings) |
| `-Wextra` | Unused params, signed/unsigned comparisons, missing field initializers |
| `-Wshadow` | Inner variable hiding an outer one of the same name |
| `-Wconversion` | Implicit narrowing that loses data |
| `-pedantic` | Non-standard g++ extensions |
| `-g` | Debug symbols (required for the debugger and readable sanitizer output) |

`.vscode/tasks.json` configured with `"problemMatcher": ["$gcc"]` so warnings populate the
Problems panel with clickable line numbers. `Ctrl+Shift+B` to build.

Also set the C/C++ extension's IntelliSense standard to `c++17`
(`Ctrl+Shift+P` → *C/C++: Edit Configurations (UI)*) so the editor and compiler agree.

**Sanitizers** (introduced, not yet used):
```bash
g++ -std=c++17 -Wall -Wextra -g -fsanitize=address,undefined -o caesar caeser_cipher.cpp
```
Catches out-of-bounds access, use-after-free, and signed overflow at runtime with a stack
trace. Would have caught Finding 6 instantly. ~2× slower. Linux/macOS work out of the box;
on Windows use WSL or MSYS2 clang.

---

## Open items carried forward

1. **`-Wshadow` on `bin-hex_converter.cpp`** — a shadowing bug was identified in the original
   converter at the very start and deliberately left unspoiled. The exercise: build that file
   with `-Wshadow`, read the warning, and determine whether it changes program behavior or is
   merely confusing. **Not yet reported back.**
2. **Strict-build output for `caeser_cipher.cpp`** — expected clean, not yet confirmed aloud.
3. **Brute-force stretch goal** — print all 25 candidate decryptions. Not attempted.

---

## Teaching workflow (for future threads)

1. Instructor issues a project, slightly harder each time
2. Instructor interviews the learner on required concepts first — checking existing knowledge
   and transferable knowledge from Python/MATLAB/Java — then teaches only the gaps
3. Abstract first, then full spec with requirements, rules, and a test matrix
4. Instructor is available for questions on the language, project, or logic
5. **Instructor never solves the problem or writes significant solution code.** Hints take the
   form of questions and trace-it-yourself exercises. When the learner is stuck, they describe
   what they tried and what happened.
6. Learner submits a working script → code review (good / bad / change)
7. Learner fixes and resubmits, iteratively, until clean
8. Instructor generates a context MD like this one for the project files

---

## Recurring themes to reinforce

- **Python habits that don't transfer:** copy semantics on parameters, `%` sign behavior,
  "allocation" as a mental model for locals
- **Correct by construction > correct by maintenance** — prefer structural guarantees
  (scope, `const`, single definition) over invariants a future edit can silently break
- **Explicit over implicit** — casts state a claim about range; make sure the claim is true
- **Totality** — a function should be well-defined over its entire input domain, including
  the edges nobody will type
- **Warnings are findings, not noise**

---

## Next project (planned)

Introduces `std::vector`, input validation, and splitting a string into tokens.
