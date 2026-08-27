# Learning C++

A structured, project-driven walk from "I know other languages" to "I can write idiomatic,
correct C++." Every project in this repo was written from scratch, reviewed line by line,
and revised until it was clean.

---

## Why this repo exists

I'm coming to C++ with a strong background in Python and MATLAB, some Java, and passing
exposure to C and R. That background transfers unevenly. A lot of what I already know about
loops, functions, and program structure carries over directly. A lot of what I know about
*how values behave* — copies, mutation, memory, integer arithmetic — does not, and quietly
produces wrong programs that look right.

So the goal here isn't to accumulate finished programs. It's to find the places where my
existing intuition is wrong and fix them deliberately. The code is the artifact; the review
notes are the point.

---

## Goals

- Write C++ that is **idiomatic**, not "Python with semicolons"
- Understand what the machine actually does — stack vs. heap, copies vs. references,
  what a type costs
- Build the habit of **compiling with warnings on** and treating every warning as a finding
- Reason about **totality**: a function should be well-defined across its entire input
  domain, including the edges nobody will ever type
- Prefer **correct by construction** over correct by maintenance — structural guarantees
  (scope, `const`, single definition) beat invariants a future edit can silently break
- Get comfortable with the standard library before reaching for anything else

---

## How this is run

The projects, teaching, and code reviews come from **Claude** (Anthropic's LLM), acting as
an instructor under a fixed set of rules I set up front. It is not writing this code for me,
and that constraint is the whole reason the setup works.

The arrangement lives in a persistent Claude Project called `Learn C++`, which holds every
finished `.cpp` file and every project write-up as context. That's what makes it continuous
rather than a series of disconnected chat sessions — a new thread already knows what I've
built, which concepts I've been taught, what I got wrong last time, and what's still open.
Each completed project adds its source and its `context.md` back into that context, so the
instructor's picture of where I'm at compounds instead of resetting.

**The hard rule, stated to it up front and re-stated in every write-up:**

> At no point does the instructor solve the problem or write significant solution code.

Hints come back as questions and trace-it-by-hand exercises. When I'm stuck, I describe what
I tried and what actually happened, and get asked questions rather than given answers. A
review finding gets explained — why it's a defect, what class of bug it belongs to, what
range of inputs triggers it — and then I go fix it myself. Several findings across these
projects were things I'd have never found on my own and also would have learned nothing from
if they'd just been patched for me.

Using an LLM this way is a deliberate choice about *which* kind of help is worth having.
Asking it to produce a working Caesar cipher would take one prompt and teach me nothing.
Asking it to interrogate what I already know, hand me a spec with a test matrix, and then
tear the result apart line by line is slower by an order of magnitude and is the actual
education. The failure mode of learning to program alongside an LLM is letting it do the
part that builds the skill; the rules above exist to make that structurally impossible.

Worth noting: the instructor is fallible and gets treated as such. One project's test matrix
shipped with a wrong expected value, which I only caught by checking the arithmetic myself
instead of debugging toward it. That's recorded in the write-up too. Verify the spec, don't
just satisfy it.

---

## The workflow

Each project follows the same loop. It's deliberately slow.

1. **Concept interview.** Before any code, the required concepts get checked one at a time:
   what do I already know, what transfers from another language, what's a genuine gap. Only
   the gaps get taught.
2. **Abstract, then spec.** A short description of what's being built, followed by the full
   specification: required signatures, behavior requirements, hard rules, and a test matrix
   with expected outputs.
3. **I build it.** Questions about the language, the logic, or the spec are fair game.
   Hints come back as questions and trace-it-by-hand exercises. **No solution code is ever
   written for me** — if I'm stuck, I describe what I tried and what actually happened.
4. **Code review.** What's good, what's wrong, what should change. Findings are numbered and
   explained: not just "change this" but why it's a defect and what class of bug it belongs to.
5. **Revise and resubmit.** Iteratively, until the build is clean and every finding is closed.
6. **Write it up.** Each completed project gets a context document recording what was taught,
   what the spec was, every review finding and its resolution, and what's still open.

Difficulty ratchets up each time. Each project introduces two or three new concepts and
assumes everything from the ones before it.

---

## Repo structure

```
.
├── README.md
├── bin-hex_converter.cpp          # pre-project baseline (predates the instructor rules)
├── project-NN-<name>/
│   ├── <name>.cpp                 # the deliverable
│   └── context.md                 # concepts, spec, review findings, open items
├── .vscode/
│   └── tasks.json                 # shared build task (builds whatever file is active)
└── ...
```

Each project directory is self-contained. The `context.md` is the interesting file — it's a
record of everything that was wrong on the first attempt and why.

---

## Projects

| # | Project | Source | Write-up |
|---|---|---|---|
| — | Binary ↔ Hexadecimal Converter *(pre-project baseline)* | [`bin-hex_converter.cpp`](bin-hex_converter.cpp) | — |
| 1 | Caesar Cipher | [`caeser_cipher.cpp`](project-01-caesar-cipher/caeser_cipher.cpp) | [`context.md`](project-01-caesar-cipher/context.md) |
| 2 | Numeric Statistics Tool | [`cl-num_statistics.cpp`](project-02-numeric-statistics/cl-num_statistics.cpp) | [`context.md`](project-02-numeric-statistics/context.md) |

**Binary ↔ Hexadecimal Converter** — written before this project's rules were in place;
used as the calibration point for Project 1 (its `-Wshadow` and signed/unsigned findings
were closed retroactively — see Project 2's write-up).

**Project 1 — Caesar Cipher.** First project under the instructor rules. Covers `char` as
an integer type, pass-by-value vs. reference, `const`, signed/unsigned pitfalls,
range-based for, and the sign of `%` on negative operands in C++ vs. Python.

**Project 2 — Numeric Statistics Tool.** Command-line tool that repeatedly reads a line of
comma-separated integers, validates every token by hand (no `<sstream>`, no `<algorithm>`),
and reports count/sum/min/max/mean. Introduces `std::vector`, `try`/`catch`, `std::cin`
failure/EOF states, the `bool` + out-parameter error pattern, and overflow-safe aggregation
(`long long` accumulator for `sum`). Five review rounds; all findings closed — see the
write-up for the full list, including a stack-overflow-via-recursion bug on Ctrl+D found in
round 1.

---

## Build conventions

Everything is built with warnings on. This is not optional and it's part of every
deliverable.

```bash
g++ -std=c++17 -Wall -Wextra -Wshadow -Wconversion -pedantic -g -o <name> <name>.cpp
```

| Flag | Catches |
|---|---|
| `-Wall` | Common real bugs (despite the name, *not* all warnings) |
| `-Wextra` | Unused parameters, signed/unsigned comparisons, missing initializers |
| `-Wshadow` | An inner variable hiding an outer one of the same name |
| `-Wconversion` | Implicit narrowing that silently loses data |
| `-pedantic` | Non-standard compiler extensions |
| `-g` | Debug symbols |

When something is behaving impossibly, the sanitizers come out:

```bash
g++ -std=c++17 -Wall -Wextra -g -fsanitize=address,undefined -o <name> <name>.cpp
```

These instrument the binary to catch out-of-bounds access, use-after-free, and signed
overflow at runtime, with a stack trace pointing at the exact line.

**Environment:** VS Code + g++, with a `tasks.json` build task using the `$gcc` problem
matcher so warnings land in the Problems panel with clickable line numbers.

---

## Standing rules

- **Standard library only** unless a project explicitly says otherwise
- **No magic numbers** — every constant is named
- **Brace initialization** by default (`int x { 3 };`), because it rejects narrowing conversions
- **Locals are `const` by default**; drop it only where mutation is actually needed
- **Explicit over implicit** — a cast is a claim about a value's range, so the claim had
  better be true
- **Clean build under the full flag set** is part of "done"
- Every row of the test matrix gets run before submission, not just the easy ones

---

## Recurring themes

Things that keep coming up, collected here because they're the actual content of this
exercise:

- **Habits from other languages that don't transfer.** Copy semantics on function parameters.
  The sign of `%` on negative operands. "Allocation" as a mental model for local variables.
- **Undefined behavior is not "a weird result."** It's unconstrained — the compiler is
  allowed to assume it never happens and optimize accordingly, which produces effects that
  look impossible when you trace the source by hand.
- **Signed vs. unsigned.** Unsigned types don't go negative; they wrap. In a mixed comparison
  the signed operand converts, not the other way around.
- **You can't predict the optimizer by reading source.** If performance matters, profile it.
  If it doesn't, write the clearest thing. Speculative optimization usually costs clarity,
  and the clarity was load-bearing.
- **Warnings are findings, not noise.**

---

## Status

Two projects complete (see the [Projects](#projects) table above). Each `context.md` ends
with a list of items still open; Project 2's carries forward a `median` stretch goal and a
`parse_int` rewrite without `std::stoi`, both slated for whichever future project revisits
that code.
