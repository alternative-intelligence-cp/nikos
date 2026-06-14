# Overview of the NIKOS Source Code

NIKOS v1.0.0 is a modernized fork of [NASA's IKOS](https://github.com/NASA-SW-VnV/ikos),
ported from LLVM 14 to **LLVM 20** and integrated into the Nitpick static analysis
toolchain. It provides sound, whole-program abstract-interpretation-based analysis for
C and C++ programs with an optional APRON relational domain backend.

---

## Directory Structure

```
.
├── CMakeLists.txt
├── CHANGELOG.md
├── CONTRIBUTING.md
├── CONTRIBUTORS.md
├── Dockerfile
├── LICENSE.pdf
├── LICENSE.txt
├── README.md
├── TROUBLESHOOTING.md
├── analyzer
├── ar
├── cmake
├── core
├── doc
├── docker
├── frontend
├── packaging
├── script
└── test
```

### Root Files

* [CMakeLists.txt](../CMakeLists.txt) — root CMake file; declares `project(nikos)` with
  C++17 and wires together the four sub-projects: `core`, `ar`, `frontend/llvm`, and
  `analyzer`. Minimum CMake version: 3.14. Default install prefix:
  `${CMAKE_SOURCE_DIR}/install`.

* [CHANGELOG.md](../CHANGELOG.md) — full release history since the LLVM 20 port
  (v0.2.0 through v1.0.0), following Keep a Changelog conventions.

* [CONTRIBUTING.md](../CONTRIBUTING.md) — contribution guidelines: coding standards,
  branch naming, commit messages, and pull-request process.

* [CONTRIBUTORS.md](../CONTRIBUTORS.md) — list of original IKOS contributors from NASA.

* `Dockerfile` — multi-stage Docker build that produces a self-contained NIKOS image.
  Builds LLVM 20, GMP, MPFR, PPL, and APRON from source in a builder stage; copies the
  final installed binaries into a slim runtime image.

* [LICENSE.pdf](../LICENSE.pdf) / [LICENSE.txt](../LICENSE.txt) — NASA Open Source
  Agreement (NOSA) 1.3 under which the original IKOS source is distributed.

* [README.md](../README.md) — project overview, quickstart build instructions, supported
  platforms, and links to documentation.

* [TROUBLESHOOTING.md](../TROUBLESHOOTING.md) — solutions for common build and runtime
  problems, including LLVM version mismatches, APRON link errors, and SQLite issues.

### Sub-directories

* [analyzer](../analyzer) — the analysis engine. Implements defect checkers (buffer
  overflow, null dereference, division by zero, integer overflow, etc.) on top of AR
  using abstract interpretation. Produces results in a SQLite database consumed by the
  Python wrapper scripts.

* [ar](../ar) — the **Abstract Representation** library: a typed, SSA-form intermediate
  language that is independent of any source language or compiler. All analyses operate
  on AR, making the analysis logic reusable across frontends.

* [cmake](../cmake) — CMake find-modules for all external dependencies: LLVM, Clang, GMP,
  MPFR, PPL, APRON, Boost, SQLite3, and TBB. Also contains `AddFlagUtils.cmake` and
  `HandleOptions.cmake` for build configuration.

* [core](../core) — the abstract interpretation theory library. Contains the abstract
  domain hierarchy, fixpoint iterators (WTO-based), and supporting algorithms. This
  library is header-only and has no dependency on LLVM or AR.

* [doc](../doc) — project documentation:
  - [OVERVIEW.md](OVERVIEW.md) — this file.
  - [CODING\_STANDARDS.md](CODING_STANDARDS.md) — C++ style guide.
  - [doc/install/1.0/](install/) — per-distribution installation guides for NIKOS v1.0.0.

* [docker](../docker) — Docker-related build context and helper scripts used by the
  root `Dockerfile`.

* [frontend/llvm](../frontend/llvm) — the LLVM-to-AR translator. Implements `ikos-pp`
  (the LLVM bitcode preprocessor) and `ikos-import` (the IR-to-AR importer). This is
  where the LLVM 20 porting work is concentrated: opaque pointer handling, PassManager
  migration, and debug-info translation.

* [packaging](../packaging) — distribution packaging recipes:
  - `packaging/deb/` — Debian/Ubuntu `.deb` packaging (DEBIAN control files).
  - `packaging/rpm/` — RPM spec file for Fedora/RHEL/CentOS.
  - `packaging/flatpak/` — Flatpak manifest for sandboxed distribution.

* [script](../script) — helper scripts:
  - `script/bootstrap` — rootless installation script that builds all dependencies
    (LLVM, GMP, MPFR, PPL, APRON) and NIKOS from source into a user-owned prefix.

* [test](../test) — regression and integration tests:
  - `test/install/` — smoke tests verifying the installed binary tree is functional.
  - Frontend, core, and analyzer regression tests integrated with CTest.

---

## Architecture

NIKOS operates as a four-layer stack. Each layer depends only on the layers below it,
making the system modular and the layers independently testable.

```
┌─────────────────────────────────────────────────────┐
│  Layer 4 — analyzer                                 │
│  Defect checkers, fixpoint engine, SQLite output    │
├─────────────────────────────────────────────────────┤
│  Layer 3a — frontend/llvm                           │
│  LLVM IR → AR translation (ikos-pp, ikos-import)    │
│                                                     │
│  Layer 3b — ar                                      │
│  Abstract Representation: typed SSA IR              │
├─────────────────────────────────────────────────────┤
│  Layer 2 — core                                     │
│  Abstract domain library, fixpoint iterators        │
└─────────────────────────────────────────────────────┘
```

### Layer 1 — `core`

The foundational abstract interpretation library. Provides:

- **Abstract domain interfaces** (`core/include/ikos/core/domain/`) — parameterized
  C++17 concept-style base classes for numeric, machine-integer, pointer, nullity,
  lifetime, and memory domains.
- **Fixpoint iterators** — WTO (Weak Topological Ordering) based iteration with
  widening/narrowing strategies.
- **Numeric types** — arbitrary-precision integers and rationals via GMP; machine-word
  intervals and congruences.

This layer has no dependency on LLVM, AR, or any runtime library. It can be used
independently as an abstract interpretation toolkit.

### Layer 2 — `ar`

The **Abstract Representation**: a typed, SSA-form assembly language. AR is the
language on which all NIKOS analyses are defined. Key properties:

- Language-independent — produced from LLVM IR today, but the AR grammar is not
  tied to any particular source language or compiler.
- Typed — every value, instruction, and memory reference carries explicit type
  information.
- Supports C/C++ memory model concepts: pointers, aggregates, exception landing pads.

### Layer 3a — `frontend/llvm`

Translates LLVM 20 IR into AR. Contains two executables:

- **`ikos-pp`** — preprocessor. Runs mandatory lowering passes over LLVM bitcode
  (LowerSwitch, LowerAtomic, LowerCstExpr, LowerSelect, UnifyFunctionExitNodes) and
  optional optimization passes before import. Uses the new LLVM PassManager API.
- **`ikos-import`** — importer. Walks the LLVM Module and emits a serialized AR bundle.
  Handles LLVM 20's opaque pointer model, debug-info metadata, VLAs, struct types, and
  library function signatures.

The source under `frontend/llvm/src/import/` contains the core translation logic split
by IR construct: `function.cpp`, `type.cpp`, `constant.cpp`, `bundle.cpp`, etc.

### Layer 3b — `ar`

See Layer 2 above. The `ar` sub-project provides the C++ AR object model (bundle,
function, basic block, statements) and a binary serialization format consumed by the
analyzer.

### Layer 4 — `analyzer`

The top-level analysis engine. Drives abstract interpretation over AR:

- **Value analysis** (`analyzer/src/analysis/value/`) — computes over-approximate
  invariants at every program point using the selected abstract domain.
  - *Intraprocedural* — per-function fixpoint.
  - *Interprocedural* — whole-program fixpoint with function summaries; supports both
    sequential and concurrent (TBB-parallel) variants.
- **Defect checkers** — query the computed invariants to emit warnings/errors for buffer
  overflows, null dereferences, division by zero, integer overflows, double-free, etc.
- **SQLite output** — results written to a `.db` file; the Python wrapper scripts
  (`ikos`, `ikos-report`, `ikos-view`) present these results to the user.

### Full Data Flow Pipeline

```
  C / C++ source files
         │
         ▼  clang-20 -emit-llvm
  LLVM 20 Bitcode (.bc)
         │
         ▼  ikos-pp  (ikos-pp.cpp)
  Optimized / Lowered LLVM Bitcode
         │
         ▼  ikos-import  (frontend/llvm/src/import/)
  Abstract Representation bundle (.ar)
         │
         ▼  analyzer value analysis  (analyzer/src/analysis/value/)
  Per-point abstract invariants
         │
         ▼  defect checkers
  Warnings / errors
         │
         ▼  SQLite database (.db)
  ikos-report / ikos-view  (analyzer/python/ikos/)
         │
         ▼
  Human-readable or HTML report
```

---

## Abstract Domains

NIKOS supports the following abstract domains, selectable via the `--domain` flag.

### Native Domains

These domains are implemented directly in `core/include/ikos/core/domain/` and require
no optional dependencies.

| Domain | Description |
|---|---|
| `interval` | Classic interval domain over machine integers; default domain, best balance of speed and precision for most programs. |
| `congruence` | Tracks values modulo a constant; detects alignment and parity properties invisible to interval analysis. |
| `interval-congruence` | Reduced product of interval and congruence; combines range and alignment information. |
| `gauge` | Gauge domain — expresses linear relationships between variables and loop counters; strong at loop-carried bounds. |
| `gauge-interval-congruence` | Reduced product of gauge, interval, and congruence; the strongest native domain. |
| `dbm` | Difference-Bound Matrix (DBM); relational domain tracking differences between pairs of variables. |
| `var-pack-dbm` | Variable-packing variant of DBM; partitions variables into independent packs to improve scalability. |
| `var-pack-dbm-congruence` | Reduced product of variable-packing DBM and congruence. |

### APRON Domains (optional)

These domains are provided by the [APRON](https://github.com/antoinemine/apron) numerical
abstract domain library and are available when NIKOS is built with `-DAPRON_ROOT=…`.
All APRON libraries are **statically linked** — no `LD_LIBRARY_PATH` is required at
runtime. The linker group (`-Wl,--start-group … -Wl,--end-group`) in
`cmake/FindAPRON.cmake` resolves circular dependencies between APRON component libraries.

| Domain | Description |
|---|---|
| `apron-interval` | APRON's interval domain; comparable to the native interval domain. |
| `apron-octagon` | Octagon domain; tracks linear inequalities of the form ±x ± y ≤ c — stronger than DBM, cheaper than polyhedra. |
| `apron-polka-polyhedra` | NewPolka convex polyhedra (strict inequalities); most precise relational domain. |
| `apron-polka-linear-equalities` | NewPolka linear equalities only; tracks affine equalities at lower cost than full polyhedra. |
| `apron-ppl-polyhedra` | PPL convex polyhedra; alternative polyhedra implementation via the Parma Polyhedra Library. |
| `apron-ppl-linear-congruences` | PPL linear congruences; relational congruence arithmetic. |
| `apron-pkgrid-polyhedra-lin-cong` | Cartesian product of PPL polyhedra and linear congruences via the APRON pkgrid domain. |
| `var-pack-apron-octagon` | Variable-packing octagon; partitions variables into independent packs for scalability. |
| `var-pack-apron-polka-polyhedra` | Variable-packing NewPolka convex polyhedra. |
| `var-pack-apron-polka-linear-equalities` | Variable-packing NewPolka linear equalities. |
| `var-pack-apron-ppl-polyhedra` | Variable-packing PPL polyhedra. |
| `var-pack-apron-ppl-linear-congruences` | Variable-packing PPL linear congruences. |
| `var-pack-apron-pkgrid-polyhedra-lin-cong` | Variable-packing PPL polyhedra + linear congruences product. |

> **Precision note:** `apron-octagon` passes 251/255 regression tests with 138
> `PASS_IMPROVE` results (stronger than `interval`). The 4 regressions occur on tests
> specifically designed for the `interval-congruence` or `gauge` domains.

---

## Key Components

### `core/include/ikos/core/domain/`

The abstract domain header library. Organized into sub-directories by domain category:

| Sub-directory | Contents |
|---|---|
| `numeric/` | Numeric abstract domains: `interval.hpp`, `congruence.hpp`, `gauge.hpp`, `gauge_interval_congruence.hpp`, `interval_congruence.hpp`, `dbm.hpp`, `octagon.hpp`, `var_packing_dbm.hpp`, `var_packing_dbm_congruence.hpp`, `apron.hpp` (APRON wrapper), `union.hpp`. |
| `machine_int/` | Machine-integer domains built on top of numeric domains: `interval.hpp`, `congruence.hpp`, `interval_congruence.hpp`, `polymorphic_domain.hpp` (type-erased runtime-selectable domain), `numeric_domain_adapter.hpp`. |
| `memory/` | Memory abstract domains tracking heap/stack cells. |
| `pointer/` | Pointer abstract domains (points-to sets, offsets). |
| `scalar/` | Scalar abstract domains combining numeric and pointer information. |
| `nullity/` | Null / non-null abstract domains. |
| `lifetime/` | Allocated / deallocated lifetime tracking. |
| `exception/` | Exception propagation abstract domains. |
| `abstract_domain.hpp` | Root abstract domain concept/interface. |
| `domain_product.hpp` | Generic reduced product combinator for building compound domains. |
| `separate_domain.hpp` | Map-based domain: independently tracks a separate abstract element per variable. |

### `frontend/llvm/`

The LLVM-to-AR translation layer. This is where all LLVM 20 porting work lives.

Key source files under `frontend/llvm/src/`:

| File | Purpose |
|---|---|
| `ikos_pp.cpp` | `ikos-pp` binary: runs LLVM passes to lower and optimize bitcode before import. Implements the hybrid legacy/new PassManager bridge. |
| `ikos_import.cpp` | `ikos-import` binary: drives the full IR-to-AR translation pipeline. |
| `import/function.cpp` | LLVM function and instruction translation (~100 KB); handles all LLVM 20 instruction types including opaque pointers. |
| `import/type.cpp` | LLVM type system to AR type translation (~70 KB); handles opaque pointers, VLAs, and aggregate types. |
| `import/constant.cpp` | Constant expression and global constant translation. |
| `import/bundle.cpp` | AR bundle construction: global variables, function declarations, data layout. |
| `import/library_function.cpp` | Well-known C library function signature mapping (malloc, free, printf, etc.). |
| `pass/` | Custom LLVM passes: `LowerCstExpr`, `LowerSelect`, `NameValues`, `MarkInternalInline`, `RemovePrintfCalls`, `RemoveUnreachableBlocks`. |

### `analyzer/src/analysis/value/`

The main abstract interpretation engine for value analysis.

| File / Directory | Purpose |
|---|---|
| `abstract_domain.cpp` | Constructs the concrete abstract domain instance from the `--domain` CLI argument; selects between native and APRON domains at runtime via the polymorphic domain. |
| `machine_int_domain.cpp` | Assembles the machine-integer domain used during analysis (wraps numeric domain with sign/bitwidth awareness). |
| `machine_int_domain/` | One `.cpp` file per supported domain (21 files); each registers its domain with the domain factory. |
| `global_variable.cpp` | Computes initial invariants for global variables before interprocedural analysis. |
| `interprocedural/` | Interprocedural value analysis: `sequential/` (single-threaded) and `concurrent/` (TBB-parallel). Implements the whole-program fixpoint with per-function summaries. |
| `intraprocedural/` | Intraprocedural value analysis: computes per-function invariants assuming fixed call context. |

### `analyzer/python/ikos/`

Python wrapper scripts that form the user-facing CLI. Installed alongside the C++
binaries.

| Module | Purpose |
|---|---|
| `analyzer.py` | Main `ikos` entry point; orchestrates clang compilation, `ikos-pp`, `ikos-import`, and `ikos-analyzer`; manages the analysis database. |
| `args.py` | CLI argument parsing for all `ikos` commands. |
| `report.py` | `ikos-report` implementation; formats analysis results from the SQLite database as plain text, CSV, or JSON. |
| `scan.py` | `ikos-scan` implementation; wraps `ikos` as a drop-in replacement for `make`-based build systems. |
| `view.py` | `ikos-view` implementation; serves a local HTTP server with an interactive HTML report. |
| `output_db.py` | SQLite database access layer for reading analysis results. |
| `enums.py` | Enumerations for check kinds, statuses, and safety levels — kept in sync with the C++ analyzer. |
| `settings.py.in` | CMake-configured settings file; baked-in paths to `clang-20`, `ikos-pp`, `ikos-import`, and `ikos-analyzer`. |
| `abs_int.py` | Abstract interpretation metadata helpers (domain names, widening thresholds). |
| `colors.py`, `highlight.py` | Terminal colour output and source-code syntax highlighting in reports. |

### `cmake/FindAPRON.cmake`

Locates the APRON installation and assembles the link command. Notable for two fixes
over the original NASA version:

1. **`libitvMPQ` added as a required library** — `libap_ppl` and `libap_pkgrid` depend
   on `libitvMPQ` at link time, but the original find-module did not locate it, causing
   undefined-symbol errors with static APRON builds.

2. **`-Wl,--start-group` / `-Wl,--end-group` linker groups** — the APRON domain
   libraries have circular symbol dependencies with `libapron` core. The linker group
   forces the GNU linker to make multiple passes over the archive group, resolving all
   symbols without requiring the caller to specify libraries in a hand-crafted order.

Search path priority: `APRON_ROOT` CMake variable → `APRON_INSTALL` environment variable
→ system default paths.

---

## Installation Guides

Platform-specific installation guides for NIKOS v1.0.0 are located in
[`doc/install/1.0/`](install/). Guides are provided for:

- Ubuntu (20.04, 22.04, 24.04)
- Debian (10, 11, 12)
- Fedora (38, 39, 40)
- Arch Linux
- macOS (Homebrew)
- Rootless / no-sudo installation (`ROOTLESS.md`)

For containerized use, see the root `Dockerfile` and the `docker/` directory.
For package-manager installation, see `packaging/deb/`, `packaging/rpm/`, and
`packaging/flatpak/`.
