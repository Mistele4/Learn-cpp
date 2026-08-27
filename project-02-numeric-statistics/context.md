# Learn C++ — Project 2 Context: Numeric Statistics Tool

**Status:** Complete
**Date:** August 2026
**Deliverable:** `cl-num_statistics.cpp`
**Builds on:** `project-01-caesar-cipher.md`, `caeser_cipher.cpp`

---

## Purpose of this document

Context carryover for future threads in the `Learn C++` project. Records what was taught,
what was built, what was found in review, and what remains open.

---

## Learner profile (updated after Project 2)

**Environment:** WSL (Ubuntu) + VS Code + g++, `tasks.json` build task configured.
Project lives under a OneDrive-synced Windows path accessed from WSL.

**Confirmed known after Project 2** (additions to the Project 1 list):
`std::vector` construction/`push_back`/`size`/`empty`/`clear`, `const T&` and `T&`
parameter selection *without prompting*, range-based for with correct value/reference
choice, `try`/`catch` by `const&`, out-parameter error reporting, `std::getline` return
value as a stream-state test, `.empty()` over `== ""`, conditional operator in a brace
initializer, function extraction as a replacement for `goto`.

**Confirmed gaps after Project 2:**
`<sstream>` and stream extraction (`>>`) beyond basic I/O, `<algorithm>`, `enum class`,
`std::optional`, iterators, sorting, operator overloading, `auto` in practice (taught,
not yet used), file I/O, classes/structs, `reserve`/capacity tuning in real use.

**Recurring habit to keep watching:** `const` on locals is still not automatic — it was
applied only after being flagged, in three consecutive review rounds. Same for
value-vs-reference in range-based for.

---

## Closed from Project 1

1. **`-Wshadow` on `bin-hex_converter.cpp`** — done. Two warnings found: a signed/unsigned
   comparison between `int i` and `str.length()`, and the shadowed loop variable in
   `hex_to_bin`. Diagnosis reached: the shadowing was **confusing, not wrong** — the inner
   `int i` is a distinct variable in a nested scope, so the outer `i` is untouched when the
   inner loop ends. Renaming to `j` changed nothing observable, which is the confirmation.
   The value of the finding is that correctness depended on a scoping rule the reader must
   stop and verify.
   - **Follow-up teaching point:** the signed/unsigned warning was fixed by
     `static_cast<int>(str.length())`. That silences it but *makes a claim* — "this length
     fits in an `int`" — that is true for typed input and false for a 4 GB string. Declaring
     `std::size_t i` makes the claim unnecessary. **Prefer matching the type over casting.**
2. **Strict-build output for `caeser_cipher.cpp`** — confirmed clean.

---

## Concepts taught in Project 2

### 1. `std::vector`
Dynamically sized, **contiguous**, **homogeneous** array. Homogeneity is what buys the
contiguity and the speed.

- vs. Python `list`: no mixed types, no negative indexing (`v[-1]` is an out-of-bounds
  read, not "last"), no slicing, no `in`, no `.sort()` without `<algorithm>`
- **size vs. capacity:** `push_back` at `size == capacity` allocates a new (typically 2×)
  buffer, moves every element, frees the old one. Geometric growth ⇒ amortized O(1).
- **Reallocation invalidates every pointer, reference, and iterator into the vector.**
- `v.reserve(n)` pre-allocates when the count is known.
- Parameters follow Project 1 unchanged: `const std::vector<T>&` read-only,
  `std::vector<T>&` to modify the caller's.
- **`return v;` by value is the correct default** — NRVO constructs in place, and move
  semantics steal the buffer when it can't. Do not contort a signature into an
  out-parameter for "performance."

| | out-of-range index |
|---|---|
| `v[i]` | **UB.** No check. May appear to work. |
| `v.at(i)` | throws `std::out_of_range`. Costs a comparison. |

Rule: `v[i]` when the index provably comes from your own loop bounds; `.at(i)` when it
came from outside.

### 2. `std::string::npos` and `substr`
- `find` returns `std::size_t`; failure is `std::string::npos` (the max `std::size_t`,
  18446744073709551615). It does **not** throw and is **not** `-1`.
- `if (str.find(c) >= 0)` is always true — unsigned. Only correct test is `== npos`.
- `find(c, start)` takes a start offset.
- **`substr(pos, count)` takes a LENGTH, not an end index** — diverges from Python slicing.
  `substr(pos)` runs to the end. `pos == size()` is legal (empty result); `pos > size()`
  throws.

### 3. `std::stoi` is more permissive than most specs

| Call | Result |
|---|---|
| `std::stoi("42")` | `42` |
| `std::stoi("  42")` | `42` — leading whitespace skipped silently |
| `std::stoi("12abc")` | **`12`** — parses the prefix, does not complain |
| `std::stoi("abc")` / `std::stoi("")` | throws `std::invalid_argument` |
| `std::stoi("99999999999999")` | throws `std::out_of_range` |

**Library leniency is the caller's problem to tighten.** An uncaught exception calls
`std::terminate` — not a return code you can ignore.

### 4. `try` / `catch` / `throw` — divergences from Java
- **No checked exceptions.** No `throws` clause; nothing at the call site reveals that
  `stoi` can throw. You must read the docs.
- **No `finally`.** RAII instead — destructors run during unwinding, so a `std::vector`
  frees itself on either path.
- **Catch by `const&`.** By value slices; by pointer is a memory-management mess.
- Handlers are tried in order; first match wins, so most-derived first.
- `throw 42;` is legal. Don't. Throw something derived from `std::exception`.
- **Style:** exceptions are for *exceptional* conditions. "The user typed a non-number" is
  not exceptional in an interactive program. Catch at the boundary, convert to `false`.

### 5. `std::cin` failure state
`std::cin >> n;` where `n` is `int` and the user types `hello`:
- `n` is set to **`0`** (C++11+), not left unchanged
- **`failbit`** is set — the stream is *failed*, not closed
- `hello\n` is **still in the buffer**; `>>` consumed nothing
- every later read is a **no-op** while failbit is set ⇒ classic infinite loop

Recovery is two steps:
```cpp
std::cin.clear();
std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
```

**EOF is a third state.** Ctrl+D / Ctrl+Z makes `std::getline(std::cin, line)` return a
stream that tests `false` with `line` empty.

> **The distinction that caused confusion in this project:** an EOF and a genuinely empty
> line produce the *same string*. They are told apart by **`getline`'s return value**, not
> by inspecting the string.
> ```cpp
> if (!std::getline(std::cin, inp)) { break; }   // EOF: channel gone, exit
> // past here the read succeeded; empty inp is a real empty line → reprompt
> ```

### 6. The menu of responses to bad input
1. **Throw** — caller must catch or die. Good across API boundaries, heavy for a prompt loop.
2. **`bool` return + reference out-parameter** — caller can't ignore the bool silently.
3. **Sentinel value** (`-1`, `0`) — almost always wrong; the sentinel is a legal value.
4. **`std::optional<T>`** (C++17) — value or nothing, type-enforced. The modern answer.
5. **Assert a precondition** — declare bad input impossible; UB if the caller breaks it.
6. **Clamp / substitute a default** — silently repair; usually hides bugs.
7. **Terminate** — for unrecoverable corruption.

Underlying axis: **totality** (defined over the whole input domain ⇒ needs a way to say
"no") vs. **preconditions** (defined on a subset ⇒ caller is contractually obligated).
Neither is universally right; the correct choice depends on whether the caller can
*prove* the precondition.

### 7. `bool` is not an `int` (unlike Python)
Python's `bool` **subclasses** `int` — `True + True == 2` by inheritance. C++ `bool` is a
**distinct type** with implicit conversions both ways: `bool`→`int` gives exactly `0`/`1`;
`int`→`bool` gives `false` for `0` and **`true` for everything else, including negatives**.
- This is why `if (std::isspace(c))` is correct and `if (std::isspace(c) == 1)` is a bug —
  `isspace` returns `int` and the nonzero value is *unspecified*.
- Same mechanism gives `if (std::cin)` and `if (ptr)`.
- `bool` is a small primitive: pass by value, never `const bool&`.

### 8. `<cctype>` breadth
`std::isspace` is true for six characters: space, `\t`, `\n`, `\v`, `\f`, `\r` — broader
than "spaces and tabs." Using it *widens the contract* (it eats a stray `\r` from a
Windows-authored line, which a manual `c == ' ' || c == '\t'` does not). Fine, but it must
be a **decision with a comment**, not an accident. The Project 1 UB rule is unchanged:
`static_cast<unsigned char>` on every `<cctype>` argument.

### 9. `auto`
Compile-time type deduction, fully static — the opposite of Python's dynamic typing.
`auto x { 5 };` is an `int` forever. **Bare `auto` drops references and `const`**, so
`for (auto s : names)` copies every string; `const auto&` does not.

### 10. String temporaries
`a += b + c` calls `operator+`, which **constructs a whole new string** (heap allocation
past the ~15-char small-string buffer), copies into it, copies again into `a`, then
destroys it. `a += b; a += c;` appends into `a`'s existing buffer with no temporary.
Chained `a + b + c + d` builds three temporaries.

The reason to prefer the split form here is **not** speed on a cold path — it is that
"append this, then append that" is simpler than "build a thing, then append the thing."
Clarity and performance agree; no trade is being made.

### 11. `goto` (raised by the learner)
Real C++, not deprecated, and breaking out of a nested loop is one of its defensible uses
(the Linux kernel uses it for cleanup paths). Objections to it *here*:
1. `while (true)` + `break` + `continue` + a bottom label means the loop's exit conditions
   are no longer visible at the loop.
2. C++ **forbids jumping over the initialization of a variable with a non-trivial
   constructor** — adding a `std::string` between the `goto` and the label breaks the
   build. Works now, constrains future edits.
3. `continue_outer: ;` — the lone semicolon is load-bearing (a label must be followed by a
   statement).

**The better move is not a flag.** When you want to jump out of a nested construct, you
usually want a **function boundary** there: extract the inner loop and `return false`
*is* the goto — structured, named, and independently testable.

### 12. Include what you use
Catching `std::invalid_argument` / `std::out_of_range` requires `<stdexcept>`. It built
without it only because libstdc++ pulls it in transitively via `<string>` — an
implementation detail, not a guarantee. Breaks on a different compiler or library version.

---

## Project specification (as issued)

Command-line numeric statistics tool. Repeatedly prompt for a line of comma-separated
integers, validate them, print summary statistics. Invalid input never crashes and never
produces partial results.

Written by hand, without library conveniences: a string splitter, a whitespace trimmer, a
strict integer validator, and four aggregates over a `std::vector<int>`.

**Required signatures (exact):**
```cpp
std::vector<std::string> split(const std::string& text, char delimiter);
std::string trim(const std::string& text);
bool parse_int(const std::string& token, int& out);
long long sum(const std::vector<int>& values);
double mean(const std::vector<int>& values);
int min_value(const std::vector<int>& values);
int max_value(const std::vector<int>& values);
```

Three signatures encoded deliberate decisions:
- **`bool` + `int&` out-param** — option 2 from the menu; failure is expected, not exceptional
- **`sum` returns `long long`** — a few thousand `int`s near `INT_MAX` overflow an `int`
  accumulator, and signed overflow is UB, not wraparound. *The accumulator variable must
  also be `long long`*, not just the return type.
- **`min_value` / `max_value`, not `min` / `max`** — those names exist in `std::`, and
  unqualified calls can pull in unintended overloads via ADL. Avoid the collision rather
  than reason about it.

**Behavior:**
1. Read a whole line with `std::getline`
2. Trimmed line `quit` → exit 0
3. `getline` failure (EOF / Ctrl+D) → exit 0 cleanly, do not loop
4. Split on `,`, **keeping empty tokens** (`"1,,2"` → 3 tokens; `""` → 1 empty token)
5. Trim each token
6. Validate every token: optional leading `+`/`-`, then at least one digit, nothing else
7. **Any invalid token ⇒ name it, say why, re-prompt. No statistics, no partial output.**
8. Empty / all-whitespace line ⇒ report and re-prompt
9. On success print count, sum, min, max, mean — then loop

**Rules:**
- Headers permitted: `<iostream>`, `<string>`, `<vector>`, `<cctype>`, `<limits>`,
  `<stdexcept>`, `<iomanip>`
- **Banned:** `<sstream>` (write `split` yourself), `<algorithm>` (write the aggregate
  loops yourself) — same spirit as banning the lookup table in Project 1
- `std::stoi` permitted but must be tightened; exceptions must not escape `parse_int`
- `mean` must be a true average — `sum / count` on two integers is integer division
- **`mean` / `min_value` / `max_value` carry a documented non-empty precondition**, and
  `main` must make violating it structurally impossible
- No magic numbers, brace initialization, locals `const` by default
- Clean build under the strict flags including `-Wconversion`

---

## Test matrix

`split(text, ',')`:

| Input | Expected tokens |
|---|---|
| `a,b,c` | `a` `b` `c` (3) |
| `a` | `a` (1) |
| `` | `` (1, empty) |
| `,` | `` `` (2) |
| `a,,b` | `a` `` `b` (3) |
| `,a` | `` `a` (2) |
| `a,` | `a` `` (2) |

`trim`:

| Input | Expected |
|---|---|
| `"  42  "` | `42` |
| `"\t42\t"` | `42` |
| `"   "` / `""` | `` (empty) |
| `"4 2"` | `4 2` — interior whitespace preserved |

`parse_int`:

| Token | Result |
|---|---|
| `42` / `-42` / `+42` / `0` / `007` | true |
| `` / `abc` / `12abc` / `1.5` / `-` / `++3` | false |
| `2147483648` | false (one past `INT_MAX`) |
| `-2147483648` | true (`INT_MIN`) |

End-to-end:

| Line | Expected |
|---|---|
| `1, 2, 3` | count 3, sum 6, min 1, max 3, mean 2 |
| `1,2` | mean **1.5** |
| `-3, -7, -1` | min −7, max −1, sum −11 |
| `1,,2` | error naming the empty token, no stats |
| `1, x, 3` | error naming `x`, no stats |
| `   ` | "no values", re-prompt |
| `quit` / `  quit  ` | exit 0 |
| Ctrl+D | exit 0 |
| `2147483647, 2147483647` | sum `4294967294`, and **mean must not overflow either** |

---

## Review findings

Five review rounds. Grouped by round; all resolved.

### Round 1 — the structural round

| # | Finding | Resolution |
|---|---|---|
| 1 | **Infinite recursion + stack overflow on Ctrl+D.** `getline`'s return value was discarded, so EOF produced an empty string, which was treated as an empty *line*, which re-prompted, which hit EOF again. Spins at full CPU until it crashes. | `if (!std::getline(std::cin, inp)) { break; }` |
| 2 | **Program exited after one successful line.** `prompt()` printed stats and returned, unwinding to `main`. | Real loop |
| 3 | **Recursion used as iteration.** Every retry was a recursive `prompt()` call. C++ does not guarantee TCO, and `-O0 -g` will not do it — enough typos overflow the stack. Findings 1 and 2 were both downstream of this. | `while (true)` in `main`; retry ⇒ `continue`, quit/EOF ⇒ `break` |
| 4 | **`out += std::stoi(...)` in `parse_int`.** `+=` *reads* an out-parameter the function doesn't own. Worked only because the single caller happened to zero-initialize; `int v; parse_int(t, v);` is UB, and reusing one variable across two calls sums them. | `out =` |
| 5 | **`mean` recomputed the sum in an `int`** — reintroducing exactly the overflow the `long long` signature existed to prevent, ten lines below it. Also duplicated a tested function. | `mean` calls `sum` |
| 6 | `<stdexcept>` not included; built only via transitive include | Included |
| 7 | `parse_int` printed to `std::cout` — a validation predicate should answer a question, not talk to the user (the caller is the one that knows whether this is a prompt, a batch job, or a test) | Printing moved to the caller |
| 8 | Error message named the token but not the reason; a bare `bool` makes `""`, `"abc"`, and `"99999999999999"` indistinguishable | `std::string& error` out-parameter added |
| 9 | **`split`'s postamble was correct by maintenance** — three post-loop `if`s, one using `last_char == 0` as a stand-in for "string was empty" (wrong for an embedded `'\0'`, and two branches fire if `delimiter == '\0'`) | Collapsed to one unconditional `tokens.push_back(cur_token);` — N delimiters always produce N+1 tokens |
| 10 | `for (std::string token : tokens)` copied every string | `const std::string&` |
| 11 | `zero_idx = 48`, `nine_idx = 57` — named the numbers but named the *wrong abstraction*; ASCII codes the reader must look up, and neither is an "index" | `std::isdigit` |
| 12 | Locals named `mean` / `min` / `max` inside the functions of the same name | Renamed |
| 13 | `= ` initialization in several places; missing `const` on most locals | Braces; `const` |
| 14 | `== ""` comparisons | `.empty()` |
| 15 | `trimmed += unsure + c` built a temporary per flush | Two `+=` |
| 16 | Unreachable `if (values.empty())` | Removed |
| 17 | `bool successful { prompt() }` never read | Removed |

### Round 2 — regressions and leftovers

| # | Finding | Resolution |
|---|---|---|
| 18 | **`static_cast<unsigned char>` dropped from `isdigit`** while switching from ASCII constants. UB on bytes ≥ 128, silently reintroduced. | Restored |
| 19 | `last_char` became set-but-never-read after finding 9 | Deleted |
| 20 | `goto continue_outer;` used to escape the nested loop | Extracted the token loop into `parse_prompt_aid` returning `bool` |

### Round 3 — the error contract

| # | Finding | Resolution |
|---|---|---|
| 21 | **Two of four `return false` paths never assigned `error`**, so the user saw `Encountered: ` with nothing after it — precisely the case the parameter was added for. (The learner's diagnosis was that the variable was "out of scope"; it is a *parameter*, in scope for the whole body. The reference does carry through — nothing was ever written to it.) | Rule adopted: **every `return false` sets `error` first** |
| 22 | `e.what()` was used as the user-facing message. On libstdc++ it is literally the string `stoi`. The exception *type* carries the information, not its message. | Own wording |
| 23 | `parse_prompt_aid` left partial results in `values` on failure; `main` got away with it only by passing a fresh vector each iteration | `values.clear()` at function entry |
| 24 | `catch (const std::invalid_argument& e)` with `e` unused | Name omitted |

### Round 4 — the reachability argument

| # | Finding | Resolution |
|---|---|---|
| 25 | `stoi(sign + unsigned_token)` reassembled a string that was already exactly `token` | `stoi(token)`; `sign` deleted along with a per-call allocation |
| 26 | The `invalid_argument` catch was flagged as **possibly** dead, with a demand for a proof rather than a guess — an uncaught exception is an abort, so "it looked unreachable" is not sufficient reasoning | Proof supplied and accepted; catch removed. See below. |
| 27 | `values.clear()` was placed in `main` on a just-constructed vector — a no-op with a comment describing a hazard that didn't exist at that call site | Moved into `parse_prompt_aid` |

**Accepted proof for finding 26:** `std::stoi` throws `invalid_argument` only when *no
conversion could be performed*; it delegates to `strtol`, which accepts optional leading
whitespace, an optional `+`/`-`, then one or more digits. On reaching the `try`, the token
is proven non-empty, at most one leading sign, all remaining characters digits — a strict
subset of what `strtol` accepts. At least one digit always converts, so `invalid_argument`
is unreachable. `out_of_range` remains reachable because the validation says nothing about
magnitude. Verified empirically on `+5`.

**`-2147483648` (Project 2 answer to the Project 1-style trap):** it parses because the
sign is handed to `stoi` as part of one string, so `strtol` produces a single negative
quantity that is in range. Parsing the magnitude `2147483648` and negating it afterward
would throw `out_of_range` *before* the negation ever happened — `INT_MAX + 1`. This is the
same asymmetry that makes `<climits>` define `INT_MIN` as `-2147483647 - 1`: written as a
source literal, `-2147483648` is unary minus applied to a value that does not fit.

**Answer to the precondition question:** a precondition was correct for `mean` /
`min_value` / `max_value` because the caller can *prove* non-emptiness structurally —
`split` always returns ≥1 token and any parse failure returns early, so the vector is
never empty at the call site. It would be the **wrong** choice if the vector arrived from
a file, a socket, or another programmer's code, where nothing is provable; that situation
calls for `parse_int`'s style — a return value the caller is forced to handle. Same
function, different answer depending on who is calling.

---

## Final architecture

```
main                      loop, prompt, quit/EOF, statistics output
└─ parse_prompt_aid       tokens → values, or false (owns the user-facing error text)
   └─ parse_int           one token → int, or false + reason
      └─ trim             (applied per token by parse_prompt_aid)
split                     line → tokens
sum / mean / min_value / max_value    aggregates; mean delegates to sum
```

Note that `trim` is called twice on the same characters — once on the whole line in `main`
for the `quit`/empty checks, once per token in `parse_prompt_aid`. Harmless, and the
per-token trim is required regardless.

---

## Build configuration

Unchanged from Project 1:
```bash
g++ -std=c++17 -Wall -Wextra -Wshadow -Wconversion -pedantic -g -o cl-num_statistics cl-num_statistics.cpp
```
Clean. Sanitizers still introduced but not yet used.

---

## Open items carried forward

1. **`parse_int` stretch** — implement with no `stoi`, using `'7' - '0'` character
   arithmetic and detecting overflow *before* it occurs. Not attempted.
2. **Median stretch** — requires writing a sort by hand with `<algorithm>` banned; the
   even-length case needs the same integer-division care as `mean`. Not attempted.
3. **Configurable delimiter at runtime.** Not attempted.
4. **`token.empty()` guard at the top of `parse_int`.** Current code relies on
   `operator[]` returning `'\0'` at `size()` for const access — defined and correct, but a
   subtle guarantee the next reader must go verify.
5.**Owed explanation: `<<` and `>>`.** The learner asked for a proper treatment of the
   stream operators — that they are the bit-shift operators *overloaded* to mean something
   unrelated, and why that design is contentious. Promised for whenever operator
   overloading is covered.
6. **Deferred concepts named but not taught:** `enum class` (offered as the better design
   for `parse_int`'s error channel, declined to avoid introducing a concept late in a
   project), `std::optional`, `std::istringstream`.

---

## Teaching workflow (unchanged)

1. Instructor issues a project, slightly harder each time
2. Instructor interviews the learner on required concepts first — checking existing
   knowledge and transferable knowledge from Python/MATLAB/Java — then teaches only the gaps
3. Abstract first, then full spec with requirements, rules, and a test matrix
4. Instructor is available for questions on the language, project, or logic
5. **Instructor never solves the problem or writes significant solution code.** Hints take
   the form of questions and trace-it-yourself exercises. When the learner is stuck, they
   describe what they tried and what happened.
6. Learner submits a working script → code review (good / bad / change)
7. Learner fixes and resubmits, iteratively, until clean
8. Instructor generates a context MD like this one for the project files

**Workflow note added in Project 2:** after the first full review, the learner asked for
subsequent rounds as **short numbered reminder lists without extended explanation**, with
deep explanation reserved for items they explicitly ask about. This worked well and should
be the default for later rounds of a review.

---

## Recurring themes to reinforce

Carried from Project 1, all of which recurred:
- **Correct by construction > correct by maintenance** — findings 3, 9, 21
- **Explicit over implicit; a cast is a claim** — the `bin-hex` length cast, `mean`'s casts
- **Totality** — findings 1, 21
- **Warnings are findings, not noise** — finding 19

New in Project 2:
- **A fix in one place can regress another.** `static_cast<unsigned char>` was dropped
  while addressing an unrelated finding (18), and the `invalid_argument` catch was removed
  on a hunch before the reachability proof existed (26). Re-run the whole matrix after
  every round, not just the case you were fixing.
- **Test the channel, not just the content.** Every bug in round 1 that mattered involved
  input *ending* or *repeating*, not input being wrong. Ctrl+D and "two good lines in a
  row" found more than any malformed string did.
- **Library leniency is the caller's problem.** `stoi("12abc") == 12` is not a bug in
  `stoi`.
- **Unreachable code needs a proof, not a hunch** — especially when the failure mode is
  `std::terminate`.
- **An error channel is a contract.** Adding an `error` out-parameter obligates *every*
  failure path to fill it.

---

## Next project (topics)

Introduces `struct`, file I/O (`<fstream>`), `<algorithm>` and sorting, and `enum class`.
Likely shape: read a CSV-ish file of records into a `std::vector<Record>`, validate rows,
sort by a selectable field, and write a report — with the parse/validate machinery from
this project reused rather than rewritten. This is also the natural place to pay off the
owed `<<` / `>>` explanation and to introduce `std::optional` as the alternative to the
`bool` + out-parameter pattern used here.
