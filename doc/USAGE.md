# NIKOS Usage Guide

> **NIKOS** is a modernized fork of NASA's IKOS static analyzer, rebuilt for LLVM 20.
> It uses Abstract Interpretation to prove the *absence* of bugs — not just find them.

---

## Table of Contents

1. [Quick Start](#1-quick-start)
2. [Your First Real Analysis](#2-your-first-real-analysis)
3. [Choosing Checkers](#3-choosing-checkers)
4. [Abstract Domains](#4-abstract-domains)
5. [Interprocedural vs Intraprocedural Analysis](#5-interprocedural-vs-intraprocedural-analysis)
6. [Reading the SQLite Output](#6-reading-the-sqlite-output)
7. [Integrating into CI](#7-integrating-into-ci)
8. [Common Flags Reference](#8-common-flags-reference)

---

## 1. Quick Start

### 1.1 Install Dependencies (Ubuntu 22.04 / 24.04)

NIKOS requires LLVM 20, a C++17 compiler, CMake ≥ 3.20, and a handful of libraries.

```bash
# Add the LLVM 20 apt repository
wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc
echo "deb http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-20 main" \
  | sudo tee /etc/apt/sources.list.d/llvm-20.list
sudo apt update

# Core LLVM 20 tools
sudo apt install -y \
  llvm-20 llvm-20-dev llvm-20-tools \
  clang-20 libclang-20-dev \
  lld-20

# Build tools and libraries
sudo apt install -y \
  cmake ninja-build \
  libsqlite3-dev \
  libboost-all-dev \
  libgmp-dev \
  libmpfr-dev \
  python3-pip

# Optional but recommended: Apron (for octagon/polyhedra domains)
sudo apt install -y libapron-dev
```

> **Ubuntu 22.04 note:** LLVM 20 may not be in the standard repo yet. Use the LLVM apt overlay above.
> **Ubuntu 24.04 note:** `libboost-all-dev` pulls Boost 1.83+, which is fine.

### 1.2 Clone and Build

```bash
git clone https://github.com/your-org/nikos.git
cd nikos

mkdir build && cd build

cmake .. \
  -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/usr/lib/llvm-20/lib/cmake/llvm \
  -DCMAKE_INSTALL_PREFIX=$HOME/.local

ninja -j$(nproc)
ninja install
```

Add the install prefix to your `PATH` if it isn't already:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Verify the installation:

```bash
ikos --version
# NIKOS x.y.z (IKOS fork, LLVM 20)

ikos-analyzer --version
# ikos-analyzer x.y.z
```

### 1.3 Hello World — Your First Scan

Create a tiny C file to verify everything is working:

```c
// hello.c
#include <stdio.h>

int main(void) {
    printf("Hello, NIKOS!\n");
    return 0;
}
```

Now run NIKOS end-to-end with the high-level `ikos` driver:

```bash
ikos hello.c
```

You'll see output similar to:

```
[*] Compiling hello.c
[*] Running ikos-pp
[*] Running ikos-analyzer
[*] Translating results
# Results for hello.c
# Time elapsed: 0.12 seconds

No issues found.
```

The `ikos` driver handles the entire pipeline for you:
`hello.c` → `clang-20` → `hello.bc` → `ikos-pp` → `hello.pp.bc` → `ikos-analyzer` → `hello.db`

---

## 2. Your First Real Analysis

For anything beyond toy programs you'll want to drive the pipeline manually so you can tune each stage. Let's analyze a file that actually has bugs.

### 2.1 The Target Program

```c
// buggy.c
#include <stdlib.h>
#include <string.h>

// Bug 1: possible divide-by-zero when n == 0
int average(int *arr, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += arr[i];
    }
    return sum / n;   // ← dbz: division by n, n could be 0
}

// Bug 2: out-of-bounds write
void fill(int *buf, int size) {
    for (int i = 0; i <= size; i++) {  // ← boa: i goes up to size (inclusive)
        buf[i] = i * 2;
    }
}

// Bug 3: null dereference
int use_ptr(int *p) {
    return *p;  // ← nullity: p could be NULL if caller passes NULL
}

int main(void) {
    int data[4] = {1, 2, 3, 4};
    int buf[4];

    average(data, 4);
    fill(buf, 4);       // writes buf[4] — one past the end
    use_ptr(NULL);      // null dereference

    return 0;
}
```

### 2.2 Step 1 — Compile to LLVM Bitcode

Always compile with `-O0 -g` so that NIKOS preserves variable names and source locations:

```bash
clang-20 -O0 -g -c -emit-llvm buggy.c -o buggy.bc
```

> **Why `-O0`?** Optimizations can merge or eliminate branches that NIKOS needs to reason about. Analyze unoptimized IR; let the compiler optimize separately.

### 2.3 Step 2 — Preprocess the Bitcode with `ikos-pp`

`ikos-pp` performs IKOS-specific IR transformations: it lowers intrinsics, inlines small functions, removes dead code, and inserts instrumentation hooks.

```bash
ikos-pp -opt=basic buggy.bc -o buggy.pp.bc
```

Key `-opt` levels:

| Level     | Description                                         |
|-----------|-----------------------------------------------------|
| `none`    | No transformations; useful for debugging the IR     |
| `basic`   | Inlining, mem2reg, simplifycfg — good default       |
| `aggressive` | Heavier inlining + loop unrolling; slower but more precise |

### 2.4 Step 3 — Run the Analyzer

```bash
ikos-analyzer \
  -a=boa,nullity,dbz \
  -d=interval \
  --proc=inter \
  --entry-points=main \
  buggy.pp.bc \
  -o buggy.db
```

Flag breakdown:
- `-a=boa,nullity,dbz` — enable three checkers: buffer overflow, null pointer, divide by zero
- `-d=interval` — use the Interval abstract domain (fast, good default)
- `--proc=inter` — interprocedural analysis (follows calls across functions)
- `--entry-points=main` — start analysis from `main`
- `-o buggy.db` — write results to a SQLite database

### 2.5 Step 4 — Read the Results

```bash
ikos-report buggy.db
```

```
# ikos-report: Analysis Results
# Database: buggy.db

buggy.c:6:13: warning: possible division by zero
  return sum / n;
             ^
  checker: dbz | status: warning | domain: interval

buggy.c:15:14: error: buffer overflow (out-of-bounds write)
  buf[i] = i * 2;
              ^
  checker: boa | status: error | domain: interval
  note: index i may equal 4, buffer size is 4

buggy.c:22:12: error: null pointer dereference
  return *p;
         ^
  checker: nullity | status: error | domain: interval

Summary: 1 warning, 2 errors (0 safe, 3 total checks)
```

**Understanding the status codes:**

| Status    | Meaning                                                            |
|-----------|--------------------------------------------------------------------|
| `safe`    | Proved safe — the operation **cannot** fail on any execution path |
| `warning` | Possibly unsafe — the analyzer cannot prove safety               |
| `error`   | Proved unsafe — the operation **will** fail on at least one path  |
| `unreachable` | Dead code — the check is never reached                       |

> `safe` results are NIKOS's superpower. When a check shows `safe`, it is a **mathematical proof** (within the chosen abstraction) that the bug class cannot occur.

---

## 3. Choosing Checkers

Pass a comma-separated list to `-a`. You can combine as many as you like, though more checkers = more analysis time.

| Checker ID | Full Name                        | What It Catches                                                      | When to Enable                                   |
|------------|----------------------------------|----------------------------------------------------------------------|--------------------------------------------------|
| `boa`      | Buffer Overflow Analyzer         | Out-of-bounds array reads and writes (stack, heap, global)           | Almost always — this is the most important check |
| `dbz`      | Division by Zero                 | Integer or FP divide/modulo by zero                                  | Always — near-zero cost                          |
| `nullity`  | Null Pointer Dereference         | Dereferencing a pointer that may be NULL                             | Always — cheap, catches crashes                  |
| `sio`      | Signed Integer Overflow          | Undefined behavior from signed overflow (e.g., `INT_MAX + 1`)       | Security-sensitive code, cryptography            |
| `uio`      | Unsigned Integer Overflow        | Wraparound in unsigned arithmetic                                    | Protocol parsers, size calculations              |
| `shc`      | Shift Count Checker              | Shift amounts ≥ bit width, or negative shift counts                  | Bit-manipulation code                            |
| `poa`      | Pointer Overflow Analyzer        | Pointer arithmetic that wraps around the address space               | Low-level/embedded code, kernel modules          |
| `uva`      | Uninitialized Variable Analyzer  | Use of variables before assignment                                   | Legacy C code, generated code                    |
| `dca`      | Dead Code Analyzer               | Code that is unreachable on all execution paths                      | Code quality audits                              |
| `dfa`      | Double Free Analyzer             | Calling `free()` twice on the same pointer                           | Manual memory management                         |
| `sound`    | Soundness Checker                | Flags constructs the analyzer cannot fully model (e.g., `setjmp`)   | When you need guaranteed coverage                |
| `fca`      | Format String Checker            | Mismatches between format string and arguments                       | Code using `printf`/`scanf` family               |
| `mem`      | Memory Watcher                   | Memory leaks, use-after-free (requires heap tracking)                | Long-running processes, server code              |
| `pcmp`     | Pointer Comparison               | Undefined behavior from comparing unrelated pointers                 | Strict standards compliance (MISRA, CERT)        |
| `prover`   | Property Prover                  | User-annotated `ikos_assert()` properties                            | Formal verification of specific invariants       |
| `dbg`      | Debug Checker                    | Emits internal analyzer state for debugging NIKOS itself             | NIKOS development / bug reports                  |

### Recommended Checker Sets

```bash
# Minimal — fast, catches the most common crashes
-a=boa,dbz,nullity

# Standard — good default for CI
-a=boa,dbz,nullity,sio,uva,dfa

# Security audit — covers CERT C / CWE top-25
-a=boa,dbz,nullity,sio,uio,shc,poa,uva,dfa,fca,pcmp

# Full — everything (slow, for release gating)
-a=boa,dbz,nullity,sio,uio,shc,poa,uva,dca,dfa,sound,fca,mem,pcmp,prover
```

---

## 4. Abstract Domains

The abstract domain controls how NIKOS models program state. It is the biggest lever for the **precision vs. speed** tradeoff.

### 4.1 Interval Domain (`-d=interval`)

Tracks each variable as an independent range `[lo, hi]`.

```
x ∈ [0, 10]
y ∈ [1, 5]
```

- **Speed:** Fast — typically under 1 second per 1K lines
- **Precision:** Cannot reason about relationships *between* variables
- **False positives:** Moderate — misses facts like "if x < y, then y - x > 0"
- **Best for:** Initial triage, CI gating, large codebases

```bash
ikos-analyzer -d=interval -a=boa,dbz,nullity app.pp.bc -o app.db
```

### 4.2 Apron Octagon Domain (`-d=apron-octagon`)

Tracks constraints of the form `±x ± y ≤ c` (octagons in 2D space).

```
x - y ≤ 5
x + y ≤ 20
```

- **Speed:** ~10–20× slower than interval
- **Precision:** Handles many loop induction variable relationships
- **False positives:** Much lower for array-index bounds checks
- **Best for:** Code with tight loop bounds, linear arithmetic over indices

```bash
ikos-analyzer -d=apron-octagon -a=boa app.pp.bc -o app.db
```

> Requires NIKOS built with `-DWITH_APRON=ON` and `libapron-dev` installed.

### 4.3 Apron Polka Polyhedra Domain (`-d=apron-polka-polyhedra`)

Tracks arbitrary linear inequalities (convex polyhedra).

```
3x + 2y ≤ 100
x ≥ 0, y ≥ 0
```

- **Speed:** Very slow — can be 100× or more slower than interval
- **Precision:** Maximum achievable with convex abstract interpretation
- **False positives:** Minimal for linear arithmetic
- **Best for:** Safety-critical, small but complex functions, formal proofs

```bash
ikos-analyzer -d=apron-polka-polyhedra -a=boa app.pp.bc -o app.db
```

### 4.4 Domain Decision Table

| Situation                                              | Recommended Domain         |
|--------------------------------------------------------|----------------------------|
| First pass / quick sanity check                        | `interval`                 |
| CI pipeline (< 5 min budget)                           | `interval`                 |
| Too many false positives from interval                 | `apron-octagon`            |
| Loop bounds are linear expressions of other variables  | `apron-octagon`            |
| Safety-critical, need minimal false positives          | `apron-polka-polyhedra`    |
| Code < ~2K IR instructions and precision matters most  | `apron-polka-polyhedra`    |
| Codebase > 100K lines                                  | `interval` (scale matters) |
| Release gate with more time budget                     | `apron-octagon`            |

### 4.5 Domain Comparison Example

Consider this loop:

```c
void copy(int *dst, int *src, int n) {
    for (int i = 0; i < n; i++) {
        dst[i] = src[i];  // Is this safe?
    }
}
```

- **Interval:** Warns — cannot prove `i < n` means `i` is in bounds without knowing `n` bounds
- **Octagon:** Often proves safe — tracks `i ≤ n-1` and correlates with buffer size
- **Polyhedra:** Proves safe — full linear arithmetic captures the relationship precisely

---

## 5. Interprocedural vs Intraprocedural Analysis

### 5.1 Intraprocedural (`--proc=intra`)

Analyzes each function **independently** in isolation. When it encounters a call to another function, it uses a **conservative summary**: it assumes the callee can return any value and may have modified any pointer argument.

```bash
ikos-analyzer --proc=intra -a=boa,dbz,nullity app.pp.bc -o app.db
```

**Characteristics:**
- Fast — linear in the number of functions
- Many false positives at call sites
- Good for: large codebases where interprocedural analysis is too slow, library analysis where callees are unknown
- Each function is independently verifiable (useful for unit-level proofs)

### 5.2 Interprocedural (`--proc=inter`)

Follows calls across function boundaries, propagating facts from callers to callees and return values back up.

```bash
ikos-analyzer --proc=inter -a=boa,dbz,nullity app.pp.bc -o app.db
```

**Characteristics:**
- More precise — tracks value flow through calls
- Can be significantly slower for deeply-nested or recursive call graphs
- Eliminates many false positives caused by cross-function value constraints
- Handles recursion with widening (may lose some precision)

### 5.3 When Each Matters

Consider this pattern, which is extremely common:

```c
static int checked_divide(int a, int b) {
    if (b == 0) return -1;   // guard
    return a / b;             // safe division
}

int main(void) {
    int result = checked_divide(10, 0);
    // ...
}
```

- **Intraprocedural:** `checked_divide` is analyzed in isolation. The `a / b` is flagged as a possible divide-by-zero because `b` is an unknown parameter.
- **Interprocedural:** The analyzer tracks that `main` calls `checked_divide(10, 0)`, propagates `b=0` into the callee, and can determine the early-return branch is taken — so `a / b` is `unreachable`, not a warning.

### 5.4 Tradeoffs Summary

| Aspect                  | `--proc=intra`           | `--proc=inter`                  |
|-------------------------|--------------------------|---------------------------------|
| Speed                   | Fast                     | Slower (depends on call depth)  |
| Precision at call sites | Low (conservative)       | High                            |
| False positives         | More                     | Fewer                           |
| Handles recursion       | N/A (each fn isolated)   | Yes, with widening              |
| Memory usage            | Low                      | Higher (call graph state)       |
| Scalability             | Very large codebases     | Medium codebases                |
| Best for                | Libraries, initial pass  | Applications with entry point   |

> **Practical tip:** Start with `--proc=inter` for application code (you have an entry point). Switch to `--proc=intra` only when analysis time becomes prohibitive or for analyzing standalone library functions.

### 5.5 Controlling Entry Points

With `--proc=inter`, always specify your entry point(s) to bound the analysis:

```bash
# Single entry point
ikos-analyzer --proc=inter --entry-points=main ...

# Multiple entry points (e.g., a library with multiple public APIs)
ikos-analyzer --proc=inter --entry-points=api_init,api_process,api_cleanup ...

# Analyze all functions as if they are entry points (intra-style, but with summaries)
ikos-analyzer --proc=inter --entry-points=__all__ ...
```

---

## 6. Reading the SQLite Output

NIKOS writes all analysis results to a SQLite database. This makes results scriptable, diffable, and easy to integrate into dashboards.

### 6.1 Key Tables

```sql
-- Open the database
sqlite3 buggy.db

.tables
-- checks  statements  operands  functions  call_contexts  settings
```

#### `checks` — The Main Results Table

```sql
SELECT * FROM checks LIMIT 5;
```

| Column        | Type    | Description                                             |
|---------------|---------|---------------------------------------------------------|
| `id`          | INTEGER | Primary key                                             |
| `checker`     | TEXT    | Checker that produced this result (`boa`, `dbz`, etc.) |
| `status`      | TEXT    | `safe`, `warning`, `error`, `unreachable`               |
| `statement_id`| INTEGER | FK → `statements.id`                                   |
| `operand_id`  | INTEGER | FK → `operands.id` (which operand triggered the check) |
| `info`        | TEXT    | Human-readable explanation                             |

#### `statements` — Source Locations

```sql
SELECT * FROM statements LIMIT 5;
```

| Column       | Type    | Description                                |
|--------------|---------|--------------------------------------------|
| `id`         | INTEGER | Primary key                                |
| `function_id`| INTEGER | FK → `functions.id`                        |
| `file`       | TEXT    | Source file path                           |
| `line`       | INTEGER | Line number                                |
| `col`        | INTEGER | Column number                              |
| `kind`       | TEXT    | IR statement kind (store, load, call, ...) |

#### `functions` — Function Metadata

```sql
SELECT * FROM functions;
```

| Column   | Type    | Description                    |
|----------|---------|--------------------------------|
| `id`     | INTEGER | Primary key                    |
| `name`   | TEXT    | Mangled function name          |
| `file`   | TEXT    | Source file                    |
| `line`   | INTEGER | Declaration line               |

#### `operands` — Operand Details

```sql
SELECT * FROM operands LIMIT 5;
```

| Column | Type    | Description                                      |
|--------|---------|--------------------------------------------------|
| `id`   | INTEGER | Primary key                                      |
| `repr` | TEXT    | String representation of the operand            |
| `type` | TEXT    | Type (e.g., `i32`, `i64*`)                       |

### 6.2 Using `ikos-report`

The `ikos-report` tool is the friendliest way to view results:

```bash
# Default: show warnings and errors only
ikos-report buggy.db

# Show all checks including safe ones
ikos-report --display-checks=all buggy.db

# Filter to a specific checker
ikos-report --display-checks=warning,error --checker=boa buggy.db

# Output as JSON (for tooling integration)
ikos-report --format=json buggy.db > results.json

# Output as CSV
ikos-report --format=csv buggy.db > results.csv
```

### 6.3 Example SQL Queries

These queries are useful for custom reporting and CI integration:

```sql
-- Count checks by status
SELECT status, COUNT(*) AS count
FROM checks
GROUP BY status
ORDER BY count DESC;

-- All errors with file and line number
SELECT
    c.checker,
    s.file,
    s.line,
    s.col,
    c.info
FROM checks c
JOIN statements s ON c.statement_id = s.id
WHERE c.status = 'error'
ORDER BY s.file, s.line;

-- All warnings for a specific checker (boa)
SELECT
    s.file,
    s.line,
    c.info
FROM checks c
JOIN statements s ON c.statement_id = s.id
WHERE c.checker = 'boa'
  AND c.status IN ('warning', 'error')
ORDER BY s.file, s.line;

-- Functions with the most warnings (hotspots)
SELECT
    f.name,
    f.file,
    COUNT(*) AS warning_count
FROM checks c
JOIN statements s ON c.statement_id = s.id
JOIN functions f ON s.function_id = f.id
WHERE c.status = 'warning'
GROUP BY f.id
ORDER BY warning_count DESC
LIMIT 10;

-- Percentage of checks that are safe (soundness metric)
SELECT
    ROUND(100.0 * SUM(CASE WHEN status = 'safe' THEN 1 ELSE 0 END) / COUNT(*), 1)
        AS pct_safe,
    COUNT(*) AS total
FROM checks;
```

### 6.4 Diffing Two Runs

Compare results between two commits using SQLite:

```bash
# Baseline (main branch)
ikos-analyzer ... -o baseline.db

# New results (your branch)
ikos-analyzer ... -o new.db

# Find regressions: warnings in new that don't appear in baseline
sqlite3 new.db "
ATTACH 'baseline.db' AS base;

SELECT n.checker, ns.file, ns.line, n.info
FROM checks n
JOIN statements ns ON n.statement_id = ns.id
WHERE n.status IN ('warning', 'error')
  AND NOT EXISTS (
    SELECT 1 FROM base.checks b
    JOIN base.statements bs ON b.statement_id = bs.id
    WHERE b.checker = n.checker
      AND bs.file = ns.file
      AND bs.line = ns.line
      AND b.status IN ('warning', 'error')
  );
"
```

---

## 7. Integrating into CI

### 7.1 Exit Codes

`ikos-analyzer` exits with a code that reflects the analysis outcome:

| Exit Code | Meaning                                                  |
|-----------|----------------------------------------------------------|
| `0`       | All checks passed (all results are `safe` or `unreachable`) |
| `N > 0`   | N checks resulted in `warning` or `error`               |
| `1`       | Fatal error in the analyzer itself (crash, bad input)   |

> `ikos-report` respects the same convention — use its exit code to gate CI.

### 7.2 Filtering for CI

In CI you typically want to fail only on **errors** (proved bugs), not warnings:

```bash
ikos-report --display-checks=error buggy.db
EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
    echo "NIKOS found $EXIT_CODE proven bugs. Blocking merge."
    exit 1
fi
echo "NIKOS: no proven bugs found."
```

For stricter enforcement, also fail on warnings:

```bash
ikos-report --display-checks=warning,error buggy.db || exit 1
```

### 7.3 Combining with Clang Warnings

NIKOS and clang warnings are complementary. A good CI script runs both:

```bash
#!/usr/bin/env bash
set -e

SRC="src/main.c"
BC="build/main.bc"
PP="build/main.pp.bc"
DB="build/main.db"

echo "=== clang warnings ==="
clang-20 -Wall -Wextra -Wpedantic -Werror \
  -O0 -g -c "$SRC" -o /dev/null

echo "=== NIKOS static analysis ==="
clang-20 -O0 -g -c -emit-llvm "$SRC" -o "$BC"
ikos-pp -opt=basic "$BC" -o "$PP"
ikos-analyzer \
  -a=boa,dbz,nullity,sio,uva,dfa \
  -d=interval \
  --proc=inter \
  --entry-points=main \
  "$PP" -o "$DB"

# Fail on proven bugs
ikos-report --display-checks=error "$DB"
echo "=== All checks passed ==="
```

### 7.4 GitHub Actions Snippet

```yaml
# .github/workflows/nikos.yml
name: NIKOS Static Analysis

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  nikos:
    runs-on: ubuntu-24.04

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install LLVM 20 and dependencies
        run: |
          wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key \
            | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc
          echo "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-20 main" \
            | sudo tee /etc/apt/sources.list.d/llvm-20.list
          sudo apt update
          sudo apt install -y llvm-20 llvm-20-dev clang-20 \
            libsqlite3-dev libboost-all-dev libgmp-dev libapron-dev \
            cmake ninja-build

      - name: Cache NIKOS build
        uses: actions/cache@v4
        with:
          path: ~/.local
          key: nikos-${{ runner.os }}-${{ hashFiles('CMakeLists.txt') }}

      - name: Build NIKOS
        run: |
          mkdir -p build && cd build
          cmake .. -GNinja \
            -DCMAKE_BUILD_TYPE=Release \
            -DLLVM_DIR=/usr/lib/llvm-20/lib/cmake/llvm \
            -DCMAKE_INSTALL_PREFIX=$HOME/.local \
            -DWITH_APRON=ON
          ninja -j$(nproc)
          ninja install
          echo "$HOME/.local/bin" >> $GITHUB_PATH

      - name: Compile to bitcode
        run: |
          clang-20 -O0 -g -c -emit-llvm src/main.c -o main.bc
          ikos-pp -opt=basic main.bc -o main.pp.bc

      - name: Run NIKOS analyzer
        run: |
          ikos-analyzer \
            -a=boa,dbz,nullity,sio,uva,dfa \
            -d=interval \
            --proc=inter \
            --entry-points=main \
            main.pp.bc -o main.db

      - name: Report results
        run: ikos-report --display-checks=all main.db

      - name: Upload NIKOS database
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: nikos-results
          path: main.db

      - name: Fail on proven bugs
        run: |
          ERROR_COUNT=$(sqlite3 main.db \
            "SELECT COUNT(*) FROM checks WHERE status='error';")
          if [ "$ERROR_COUNT" -gt 0 ]; then
            echo "::error::NIKOS found $ERROR_COUNT proven bug(s). See report above."
            exit 1
          fi
          echo "NIKOS: no proven bugs. All clear."
```

> **Tip:** Upload the `.db` artifact so developers can run `ikos-report` locally against the CI results without re-running the analysis.

### 7.5 Makefile Integration

For projects using Make:

```makefile
# Makefile excerpt
CLANG    := clang-20
IKOS_PP  := ikos-pp
IKOS     := ikos-analyzer
REPORT   := ikos-report

SRCS     := $(wildcard src/*.c)
BCS      := $(SRCS:src/%.c=build/%.bc)
PP_BCS   := $(SRCS:src/%.c=build/%.pp.bc)

CHECKERS := boa,dbz,nullity,sio,uva
DOMAIN   := interval

.PHONY: analyze clean-analysis

build/%.bc: src/%.c
	$(CLANG) -O0 -g -c -emit-llvm $< -o $@

build/%.pp.bc: build/%.bc
	$(IKOS_PP) -opt=basic $< -o $@

build/analysis.db: $(PP_BCS)
	$(IKOS) \
	  -a=$(CHECKERS) \
	  -d=$(DOMAIN) \
	  --proc=inter \
	  --entry-points=main \
	  $^ -o $@

analyze: build/analysis.db
	$(REPORT) --display-checks=all $<

analyze-strict: build/analysis.db
	$(REPORT) --display-checks=error $<

clean-analysis:
	rm -f build/*.bc build/*.pp.bc build/analysis.db
```

---

## 8. Common Flags Reference

### 8.1 `ikos-analyzer` Flags

| Flag                        | Values / Default             | Description                                                                          |
|-----------------------------|------------------------------|--------------------------------------------------------------------------------------|
| `-a`, `--analyses`          | `boa,dbz,nullity,...`        | Comma-separated list of checkers to enable                                           |
| `-d`, `--domain`            | `interval` (default)         | Abstract domain: `interval`, `apron-octagon`, `apron-polka-polyhedra`               |
| `--proc`                    | `inter` / `intra`            | Interprocedural or intraprocedural mode                                             |
| `--entry-points`            | `main` (default)             | Comma-separated list of function names to use as analysis roots                     |
| `--widening-delay`          | Integer, default `2`         | Number of loop iterations before applying widening (higher = more precise, slower)  |
| `--widening-strategy`       | `widen-then-narrow`          | Widening algorithm; `widen-then-narrow` usually gives best precision                |
| `--no-fixpoint-cache`       | Flag (off by default)        | Disable caching of function summaries — use when debugging analysis correctness     |
| `--display-checks`          | `all`, `warning`, `error`    | Which check statuses to display in terminal output                                  |
| `--display-invariants`      | `all`, `fail`, `none`        | Print abstract state at each program point (verbose, for debugging)                 |
| `-o`, `--output`            | Path to `.db` file           | Output SQLite database path                                                         |
| `--no-color`                | Flag                         | Disable ANSI colors (useful for CI log parsers)                                     |
| `--log`                     | `info`, `debug`, `warning`   | Verbosity of analyzer log messages                                                  |
| `--inline-all`              | Flag                         | Aggressively inline all function calls before analysis (increases precision)        |
| `--globals-init`            | `top`, `zero`, `skip`        | How to initialize global variables: `zero` matches C standard, `top` is conservative|
| `--hardware-addresses`      | Flag                         | Model hardware-mapped memory regions (embedded/kernel code)                         |

### 8.2 `ikos-pp` Flags

| Flag            | Values / Default      | Description                                                    |
|-----------------|-----------------------|----------------------------------------------------------------|
| `-opt`          | `none`, `basic`, `aggressive` | Optimization level for IR preprocessing                |
| `--inline`      | Flag                  | Force inlining of all calls (same as aggressive's inline pass) |
| `--verify`      | Flag                  | Run LLVM verifier after transformations (sanity check)         |
| `-o`            | Output file path      | Output `.pp.bc` file                                           |

### 8.3 `ikos-report` Flags

| Flag                 | Values / Default                | Description                                             |
|----------------------|---------------------------------|---------------------------------------------------------|
| `--display-checks`   | `all`, `warning,error`, `error` | Filter which statuses to show                           |
| `--checker`          | Checker ID (e.g., `boa`)        | Filter to a specific checker                            |
| `--format`           | `text`, `json`, `csv`           | Output format                                           |
| `--no-color`         | Flag                            | Disable colored output                                  |
| `--file`             | Source file path                | Filter results to a specific source file                |
| `--function`         | Function name                   | Filter results to a specific function                   |

### 8.4 Tuning Widening for Better Precision

Widening is the mechanism that ensures loop analysis terminates. A `--widening-delay` of `N` means the analyzer runs `N` exact iterations before switching to widening (which may over-approximate).

```bash
# Default: widen after 2 iterations (fast, may miss tight bounds)
ikos-analyzer --widening-delay=2 ...

# More precise: run 5 exact iterations first
ikos-analyzer --widening-delay=5 ...

# For heavily unrolled or simple loops, delay=1 is sufficient
ikos-analyzer --widening-delay=1 ...
```

> Doubling `--widening-delay` roughly doubles analysis time for loop-heavy code. Values above 10 rarely improve results.

---

## Appendix A: Full Pipeline Cheat Sheet

```
┌─────────────┐    clang-20      ┌──────────┐    ikos-pp      ┌────────────┐
│  source.c   │  ─────────────►  │ source.bc│  ──────────►  │ source.pp.bc│
│  (C/C++)    │  -O0 -g          │ (LLVM IR)│  -opt=basic    │(preprocessed│
└─────────────┘  -c -emit-llvm   └──────────┘                │  bitcode)  │
                                                              └─────┬──────┘
                                                                    │
                                                         ikos-analyzer
                                                         -a=boa,dbz,nullity
                                                         -d=interval
                                                         --proc=inter
                                                                    │
                                                              ┌─────▼──────┐
                                                              │  results.db │
                                                              │  (SQLite)  │
                                                              └─────┬──────┘
                                                                    │
                                                            ikos-report
                                                                    │
                                                        ┌───────────▼──────────┐
                                                        │  Human-readable      │
                                                        │  warnings/errors     │
                                                        │  + exit code         │
                                                        └──────────────────────┘
```

## Appendix B: Checker × Domain Compatibility

| Checker  | Interval | Octagon | Polyhedra | Notes                                        |
|----------|----------|---------|-----------|----------------------------------------------|
| `boa`    | ✓        | ✓✓      | ✓✓✓       | Benefits most from relational domains        |
| `dbz`    | ✓        | ✓       | ✓         | Interval usually sufficient                  |
| `nullity`| ✓        | ✓       | ✓         | Interval usually sufficient                  |
| `sio`    | ✓        | ✓✓      | ✓✓✓       | Relational domains help with overflow guards |
| `uio`    | ✓        | ✓✓      | ✓✓✓       | Same as sio                                  |
| `uva`    | ✓        | ✓       | ✓         | Domain choice rarely matters                 |
| `dfa`    | ✓        | ✓       | ✓         | Domain choice rarely matters                 |
| `mem`    | ✓        | ✓✓      | ✓✓        | Octagon helps with aliased pointer ranges    |

✓ = supported, ✓✓ = beneficial, ✓✓✓ = strongly recommended

## Appendix C: Troubleshooting

### "ikos-analyzer: out of memory"

```bash
# 1. Switch to a lighter domain
-d=interval

# 2. Use intraprocedural mode
--proc=intra

# 3. Reduce widening delay
--widening-delay=1

# 4. Analyze one translation unit at a time instead of linking everything
```

### "ikos-pp: cannot lower intrinsic"

Some LLVM intrinsics (e.g., from SIMD or crypto builtins) may not be handled. Try:

```bash
# Compile without target-specific extensions
clang-20 -O0 -g -c -emit-llvm -mno-sse -mno-avx source.c -o source.bc
```

### Too many false positives on external library calls

If your code calls libc/libstd functions that NIKOS doesn't have models for, it will conservatively assume the worst. Use `--globals-init=zero` (matches C standard semantics) and provide stubs:

```c
// stubs.c — provide conservative-but-precise models
void *malloc(size_t n) {
    // NIKOS will use its built-in model; this file isn't needed
    // unless you have custom allocators
}
```

For third-party libraries, link their bitcode with your bitcode before analysis:

```bash
clang-20 -O0 -g -c -emit-llvm mylib.c -o mylib.bc
clang-20 -O0 -g -c -emit-llvm main.c  -o main.bc
llvm-link-20 main.bc mylib.bc -o combined.bc
ikos-pp -opt=basic combined.bc -o combined.pp.bc
ikos-analyzer --entry-points=main combined.pp.bc -o results.db
```

### Analysis takes too long

Profile which functions consume the most time:

```bash
ikos-analyzer --log=info ... 2>&1 | grep "Analyzing function"
```

Then selectively exclude known-safe third-party functions from the analysis entry point, or switch to `--proc=intra` for the expensive ones.

---

*This guide covers NIKOS as of the LLVM 20 era. For the latest flags, run `ikos-analyzer --help` and `ikos-pp --help`.*
