# LLVM 14 → 20: AR Output Changes

This document describes how the **Abstract Representation (AR)** output from
`ikos-import` changes when moving from LLVM 14 to LLVM 20. It is essential
reading if you are:

- **Adding or updating regression tests** in `frontend/llvm/test/regression/import/`
- **Porting a downstream tool** that consumes NIKOS's AR text output
- **Debugging CI failures** after an LLVM toolchain upgrade
- **Contributing patches** that touch the LLVM→AR translation layer

---

## Background

NIKOS's regression tests embed `; CHECK:` lines directly into `.ll` (LLVM IR)
source files. The test runner pipes `ikos-import` output through LLVM's
[FileCheck](https://llvm.org/docs/CommandGuide/FileCheck.html) tool to verify
the AR matches expectations. Because the AR reflects LLVM's internal IR
structure, upgrading LLVM changes the AR in ways that require updating those
CHECK lines.

The changes below were discovered and documented during the LLVM 14 → 20
migration of NIKOS.

---

## Critical: Local vs CI LLVM Version

> [!WARNING]
> **The system `ikos-import` may be built against a different LLVM version than CI.**
> On Ubuntu 24.04, `/usr/local/bin/ikos-import` may be LLVM 14 while CI builds
> against LLVM 20. Running tests with the wrong binary produces misleading results —
> tests may appear to pass locally but fail in CI, or vice versa.

Always verify the version before running tests:

```bash
ikos-import --version
# Should print: Ubuntu LLVM version 20.x.x
```

If you have a CI-built binary (e.g., from a `build_ci_test/` artifact), use it:

```bash
# Run a single test with the LLVM 20 binary
build_ci_test/frontend/llvm/ikos-import \
  -format=text -order-globals -allow-dbg-mismatch \
  frontend/llvm/test/regression/import/no_optimization/reference.ll \
  | /usr/lib/llvm-20/bin/FileCheck \
    frontend/llvm/test/regression/import/no_optimization/reference.ll

# Run the full suite for one optimization level
cd frontend/llvm/test/regression/import/no_optimization
bash runtest \
  --ikos-import /path/to/llvm20/ikos-import \
  --file-check /usr/lib/llvm-20/bin/FileCheck
```

### Regenerating CHECK lines automatically

If you need to update CHECK lines after an LLVM upgrade, the recommended
approach is to regenerate them from actual `ikos-import` output rather than
editing by hand:

```bash
# Scan a directory and fix all failing tests
python3 script/regen_checks.py \
  --batch --failing-only \
  frontend/llvm/test/regression/import/no_optimization/

# Preview changes without writing (dry run)
python3 script/regen_checks.py --diff \
  frontend/llvm/test/regression/import/no_optimization/reference.ll
```

> [!IMPORTANT]
> Always verify regenerated files pass `FileCheck` before committing.
> The regeneration script assumes the AR output lines map 1:1 with CHECK lines
> in order — this holds for all current test files but may not for future ones
> with complex control flow assertions.

---

## Change 1: Scalar Alloca Types Are Now Concrete

**Before (LLVM 14)**:
```
opaque* $1 = allocate opaque, 1, align 4
```

**After (LLVM 20)**:
```
si32* $1 = allocate si32, 1, align 4
```

LLVM 14 used a transitional opaque-pointer mode where `alloca i32` didn't carry
the element type into the AR `allocate` statement. LLVM 20 fully resolves the
element type, so every scalar alloca gains a concrete type in both the variable
name and the `allocate` argument.

**Affected types**: All scalar primitives — `si8`, `si16`, `si32`, `si64`,
`ui8`, `ui16`, `ui32`, `ui64`, `float`, `double`.

**Not affected**: Pointer allocas (`alloca i32*`) remain `opaque** $N = allocate opaque*, 1, align 8`.

**Cascade**: Downstream bitcasts that converted `opaque*` → `si32*` become
same-type no-ops and are **eliminated**, shifting all subsequent SSA variable
numbers (`%N`) downward by 1 for each eliminated cast.

---

## Change 2: Struct Alloca Types Use Field Layout, Not Vector Reinterpretation

When a C struct was memory-layout-compatible with a vector type, LLVM 14 sometimes
chose the vector type for the alloca. LLVM 20 consistently uses the actual struct
field layout.

**Before (LLVM 14)** — `struct line_t { struct pos_t begin, end; }` (16 bytes):
```
{0: <2 x float>, 8: <2 x float>}* $1 = allocate {0: <2 x float>, 8: <2 x float>}, 1, align 4
```

**After (LLVM 20)**:
```
{0: {0: float, 4: float}, 8: {0: float, 4: float}}* $1 = allocate {0: {0: float, 4: float}, 8: {0: float, 4: float}}, 1, align 4
```

**Impact on stores**: When a function returns a vector-typed value and the
destination alloca is now a concrete struct, a bulk store like `store $1, %2`
is **replaced by field-by-field decomposition** using `extractelement`:

```
# LLVM 20 — decomposed store
opaque* %3 = bitcast $1
{0: <2 x float>, 8: <2 x float>}* %4 = bitcast %3
opaque* %5 = ptrshift %4, 16 * 0, 1 * 0
opaque %6 = extractelement %2, 0
<2 x float> %7 = bitcast %6
<2 x float>* %8 = bitcast %5
store %8, %7, align 4
# ... (repeated for each field)
```

---

## Change 3: Bitcast Elimination Is Selective

Not all bitcasts that appear redundant are eliminated. The pattern depends on
whether source and target types are identical in the AR type system:

| Bitcast | LLVM 14 | LLVM 20 |
|---|---|---|
| `si32* = bitcast si32*` (same type) | present | **eliminated** |
| `opaque* = bitcast si32*` (concrete → opaque, for call arg) | present | **kept** |
| `si32* = bitcast opaque*` (opaque → concrete, from load) | present | **kept** |
| `opaque** = bitcast opaque**` (pointer-to-pointer load path) | present | **kept** |

> [!NOTE]
> When in doubt about whether a bitcast will be present or absent, run
> `ikos-import` and inspect the actual output. Do not assume.

---

## Change 4: Return Value Bitcast Chain Expands

When returning a struct via an alloca that now has a concrete type (Change 2),
the bitcast chain at the return statement gains an extra step:

**Before (LLVM 14)** — alloca type matches return type directly:
```
{0: <2 x float>, 8: <2 x float>}* %16 = bitcast $2
{0: <2 x float>, 8: <2 x float>} %17 = load %16, align 4
return %17
```

**After (LLVM 20)** — intermediate opaque* required:
```
opaque* %16 = bitcast $2                                    # concrete → opaque
{0: <2 x float>, 8: <2 x float>}* %17 = bitcast %16        # opaque → vector
{0: <2 x float>, 8: <2 x float>} %18 = load %17, align 4
return %18
```

---

## Change 5: Constructor Delegation Is Not Inlined (no_optimization)

At the `no_optimization` level, C++ complete constructors (C1) that delegate to
base constructors (C2) are **not inlined**. Both LLVM 14 and LLVM 20 preserve
the delegation call. The C1 body stores arguments to locals, loads them back,
bitcasts the C2 function pointer, and calls it:

```
# Both LLVM 14 and LLVM 20 — C1 still calls C2
opaque** %10 = bitcast $5
opaque* %11 = load %10, align 8
float %12 = load $6, align 4
...
void (opaque*, float, float, float)* %15 = bitcast @_ZN3FooC2Efff
opaque* %16 = bitcast %11
call %15(%16, %12, %13, %14)
```

The difference from LLVM 14 is subtle: the **load-path bitcast** (`opaque** %10 = bitcast $5`)
is now present even though the **store-path bitcast** was eliminated. In LLVM 14,
both were present. In LLVM 20, the store uses `$5` directly but the load still
goes through the bitcast.

---

## Change 6: SSA Variable Renumbering Cascades

Because Changes 1–3 can add or remove statements, the SSA variable counter
(`%N`) shifts for every subsequent variable in the function. A single eliminated
bitcast at `%2` shifts `%3` → `%2`, `%4` → `%3`, and so on to the end of the
function body.

**Rule**: Never patch a single CHECK line in isolation. Either patch the entire
function body, or use `regen_checks.py` to regenerate from actual output.

---

## Change 7: Integer Type Signedness

Some integer types change their signedness classification in the AR:

| Context | LLVM 14 | LLVM 20 |
|---|---|---|
| `unsigned short` bitfield backing store | `ui16` | `si16` |

This was observed in `bit-field-1.ll`. The root cause is a change in how LLVM
represents the integer type for bitfield backing storage in the IR, which
propagates through to the AR type translator.

---

## Summary: What to Check When Tests Fail After an LLVM Upgrade

1. **Run `ikos-import --version`** — confirm you're testing with the right LLVM version.
2. **Get actual output**: `ikos-import -format=text -order-globals -allow-dbg-mismatch <file.ll>`
3. **Compare with CHECK lines** — look for the first mismatch.
4. **Common patterns**:
   - `allocate opaque` → `allocate si32` (Change 1)
   - Struct alloca type changed (Change 2)
   - Bitcast eliminated or added (Change 3)
   - Bitcast chain at return expanded (Change 4)
   - SSA numbers shifted by ±1 (Change 6)
5. **Use `regen_checks.py`** to regenerate automatically rather than editing by hand.
6. **Re-run FileCheck** to confirm the fix before committing.
