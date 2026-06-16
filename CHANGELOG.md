# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

**NIKOS** is a fork of [NASA's IKOS](https://github.com/NASA-SW-VnV/ikos), ported from LLVM 14 to LLVM 20 and integrated into the Nitpick static analysis toolchain.

## [2.3.1] — 2026-06-16

### 🔧 Patch Release — LLVM 20 Regression Test Suite Complete

Fixes all remaining LLVM 20 AR output mismatches across the full import
regression test suite. Adds developer tooling and documentation for future
LLVM upgrades.

### Fixed

- **Complete LLVM 20 regression test suite** — all 162 non-skipped tests now
  pass across `no_optimization`, `basic_optimization`, and
  `aggressive_optimization` suites (previously 40+ tests failing). Root cause
  was that CHECK lines were written against LLVM 14 AR output; the local
  `ikos-import` binary was LLVM 14 while CI used LLVM 20.

- **`no_optimization/`** — 37 files updated: concrete scalar alloca types
  (`allocate opaque` → `allocate si32/float/etc.`), bitcast elimination
  cascades, struct layout resolution, SSA variable renumbering.

- **`basic_optimization/`** — 3 files updated: bitfield signedness (`ui16` →
  `si16`), struct alloca type resolution, PHI node renumbering.

- **`aggressive_optimization/`** — 1 file updated: vtable bitcast chain.

### Added

- **`script/regen_checks.py`** — tool to auto-regenerate `; CHECK:` lines
  in `.ll` test files from actual `ikos-import` output. Eliminates error-prone
  hand-editing after LLVM upgrades. Supports `--batch`, `--failing-only`,
  `--diff`, and `--dry-run` modes.

- **`doc/LLVM20_AR_CHANGES.md`** — reference guide documenting every way the
  AR output changed between LLVM 14 and LLVM 20: concrete alloca types, struct
  field layout resolution, selective bitcast elimination rules, return value
  bitcast chain expansion, constructor delegation behavior, SSA renumbering
  cascades, and integer signedness changes. Includes local testing instructions
  and a troubleshooting checklist.

- **README "Working with the Test Suite" section** — links to
  `LLVM20_AR_CHANGES.md`, shows `regen_checks.py` usage, and explains how to
  run the suite locally with the correct LLVM version.

## [2.3.0] — 2026-06-15

### 🔒 Security & Taint Analysis Expansion — 14/14 Tests Passing

Third milestone in the 2.x security analysis series. Broadens taint source
coverage to network and standard I/O, ships built-in vulnerability profiles,
enhances diagnostic output with source-attribution labels, and grows the taint
regression suite from 10 to 14 tests.

### Added

- **Expanded taint sources** — `recv`, `recvfrom`, `recvmsg` (network I/O);
  `fgets`, `fgetc`, `getchar`, `getline`, `scanf`, `fscanf`, `sscanf`, `read`
  (standard I/O) registered as taint sources in `taint_config.json`.

- **New taint sinks** — exec family: `execl`, `execlp`, `execle`, `execv`,
  `execve`, `execvp` (CommandInjection); filesystem: `openat`, `unlink`,
  `rename`, `remove` (PathTraversal); `syslog` (FormatString).

- **Path sanitizers** — `realpath` and `basename` added to `sanitizers` list in
  `taint_config.json`, clearing path traversal taint when data passes through
  canonical path resolution.

- **`profiles/posix.json`** — new built-in profile targeting POSIX command/path
  injection and format string vulnerabilities; no SQL sinks.

- **Updated `profiles/web.json`** — added network I/O sources (`recv`,
  `recvfrom`, `fgets`, `read`) and `realpath` sanitizer.

- **Updated `profiles/sql.json`** — added PostgreSQL sanitizers
  (`PQescapeStringConn`, `PQescapeLiteral`) alongside existing MySQL/SQLite ones;
  broadened sources to include network and file I/O.

- **Verbosity-2 taint source labels** in `ikos-report` — at `-v 2`, taint
  findings now include `(source: <name>)` attribution drawn from the
  `taint_sources` provenance key stored in the check info dict. New helper
  `_taint_source_suffix()` in `report.py`.

- **4 new regression tests** (total: 14):
  - `recv_source.c` — network socket data → `system()` (CommandInjection)
  - `fgets_source.c` — stdin input → `fopen()` (PathTraversal)
  - `realpath_sanitizer.c` — `fgets` → `realpath()` → `fopen()` (safe)
  - `snprintf_sink.c` — `getenv()` → `snprintf()` → `system()` (CommandInjection)

### Fixed

- **`KeyError: 53` in `ikos-report`** — the system-installed `enums.py` at
  `/usr/local/libexec/lib/python3.12/site-packages/ikos/` was the old upstream
  IKOS version, missing all 2.x `CheckKind` additions (`USE_AFTER_FREE`,
  `USE_AFTER_RETURN`, `DATA_RACE`, `DEADLOCK`, `COMMAND_INJECTION`,
  `PATH_TRAVERSAL`, `FORMAT_STRING`, `SQL_INJECTION`). Deployed updated `enums.py`
  and all other modified Python files to both install targets.

## [2.2.0] — 2026-06-15

### 🧵 Concurrency Checker — Data Race & Deadlock Detection

See [RELEASE_2.2.0.md](../../../META/NIKOS/ROADMAP/2.2/RELEASE_2.2.0.md) for
full details. Highlights:
- Lockset analysis checker (`-a concurrency`) with `pthread_mutex_lock/unlock`
- `CheckKind::DATA_RACE` and `CheckKind::DEADLOCK`
- 3 regression tests (mutex_safe, data_race, deadlock)

## [2.1.0] — 2026-06-15

### 🧹 Use-After-Free Checker Enhancements

See [RELEASE_2.1.0.md](../../../META/NIKOS/ROADMAP/2.1/RELEASE_2.1.0.md) for
full details. Highlights:
- `CheckKind::USE_AFTER_FREE`, `USE_AFTER_RETURN`, `USE_AFTER_MOVE`
- `CheckerName::UAF`, `UAM`
- Lifetime tracking improvements

## [1.0.1] — 2026-06-14


### 🔧 Patch Release — CI Compatibility Fixes

Post-release patch addressing build failures discovered when CI ran against
current toolchain versions (CMake 4.3.3, AppleClang 17, Homebrew Boost 1.90).
No functional changes to the analyzer itself.

### Fixed

- **CMake 4.x compatibility** — bumped `cmake_minimum_required` from `3.4.3`
  to `3.14` in all four sub-project `CMakeLists.txt` files (`core/`, `ar/`,
  `frontend/llvm/`, `analyzer/`) and in `analyzer/python/settings.cmake.in`.
  CMake 4.3.3 removed support for `VERSION < 3.5` entirely.

- **Boost 1.90 `boost_system` removed** — `boost_system` became header-only
  in Boost 1.69 and is no longer a findable library component in Boost's own
  `BoostConfig.cmake` (shipped by Homebrew since 1.86). Removed `system` from
  `COMPONENTS` in `frontend/llvm/CMakeLists.txt` and `analyzer/CMakeLists.txt`.

- **Deprecated `FindPythonInterp`** — replaced with `find_package(Python3
  COMPONENTS Interpreter)` in `analyzer/CMakeLists.txt`. `FindPythonInterp`
  was deprecated in CMake 3.12 and removed in CMake 3.27+.

- **`wpo.hpp` member name typo** — `WpoNode::is_successor_lifted()` referenced
  `this->_successor_lifted` (non-existent); corrected to `_successors_lifted`.
  Latent bug from upstream IKOS; GCC silently accepts it via lazy template
  instantiation but AppleClang 17 hard-rejects it. Reported upstream as
  [NASA-SW-VnV/ikos#336](https://github.com/NASA-SW-VnV/ikos/issues/336).

- **`gauge.hpp` `operator=(Number)` member name typo** — `GaugeBound::operator=`
  assigned to `this->_n` (non-existent); corrected to `this->_cst`. Equivalent
  to upstream fix in [NASA-SW-VnV/ikos#332](https://github.com/NASA-SW-VnV/ikos/pull/332)
  which merged after our fork point.

- **CI workflows** — `ikos --version` verification step replaced with
  `ikos-analyzer --help` (the Python wrapper's shebang is baked to the
  install-time prefix and is not portable across CI runners). Added explicit
  `make build-*-tests` step before `ctest` since test targets are
  `EXCLUDE_FROM_ALL`.

## [1.0.0] — 2026-06-14

### 🚀 Official Production Release — 64/64 Tests Passing

NIKOS is now officially **v1.0.0 — Production Ready**. This release delivers a
complete documentation overhaul and multiple distribution methods so NIKOS can
be installed with a single command on any supported platform.

### Added

- **`script/install.sh`** — self-contained Bash install script:
  - Detects Ubuntu 22.04 / 24.04; installs LLVM 20 via `apt.llvm.org`
  - Builds from source with CMake; installs to `~/.local/nikos` (configurable)
  - `--with-apron` flag: clones, builds, and links APRON automatically
  - `--prefix`, `--jobs`, `--apron-prefix` options; idempotent (safe to re-run)

- **Debian package** (`packaging/deb/`) — `nikos_1.0.0_amd64.deb`:
  - 17 MB self-contained package with all runtime binaries
  - Built via `packaging/deb/build.sh`; installs with `dpkg -i`

- **RPM spec** (`packaging/rpm/nikos.spec`) — Fedora/RHEL RPM:
  - Standard `%prep`/`%build`/`%install`/`%files` spec for Fedora 40+
  - `packaging/rpm/README.md` with build instructions and Docker workaround for Ubuntu

- **Docker image** (`Dockerfile`) — multi-stage build:
  - Stage 1 (`builder`): Ubuntu 24.04 + LLVM 20 + full build
  - Stage 2 (`runtime`): minimal Ubuntu 24.04 with install tree only
  - `docker/docker-compose.yml` for convenience
  - `doc/DOCKER.md` with CI integration guide

- **Flatpak manifest** (`packaging/flatpak/com.alternativeintelligence.nikos.yml`):
  - Uses `org.freedesktop.Sdk.Extension.llvm20` (no LLVM source build)
  - `org.freedesktop.Platform` 24.08 runtime

- **`doc/USAGE.md`** — comprehensive walkthrough guide:
  - Full pipeline tutorial (clone → build → analyze → interpret results)
  - All 16 checker IDs with use-case guidance
  - Abstract domain selection guide with decision table
  - SQLite output schema and example SQL queries
  - GitHub Actions CI integration snippet
  - Appendices: pipeline diagram, domain compatibility matrix, troubleshooting

- **`doc/install/1.0/`** — fresh installation guides replacing the stale upstream IKOS 3.0 guides:
  - `UBUNTU_22.04.md` — primary supported platform
  - `UBUNTU_24.04.md` — notes on LLVM 17 default; LLVM apt repo required
  - `APRON.md` — deep-dive APRON source build guide (Makefile.config, OCaml skip, manual install, known issues)

### Changed

- **`CMakeLists.txt`**: project `VERSION` bumped from `0.6.0` to `1.0.0`
- **`TROUBLESHOOTING.md`**: converted from plain text to Markdown; added LLVM 20
  error table, APRON-specific issues section, fortification note, and false-positive guidance
- **`CONTRIBUTING.md`**: `make check` → `ctest`; added `LLVM_CONFIG_EXECUTABLE` to cmake invocation
- **`doc/OVERVIEW.md`**: fully rewritten for v1.0.0 — updated directory tree, expanded architecture diagram, abstract domains table, key components section
- **`doc/CODING_STANDARDS.md`**: reviewed, accurate as-is

---

## [0.13.1] — 2026-06-14


### ✅ 0.13 Series Finalization — Install Verification

Closes the v0.13 series. Verifies the APRON domains work correctly from the
installed binary tree, and confirms all 64 regression tests pass end-to-end.

### Verified

- **Install verification (NAP-022):** `ikos-analyzer` built against the install
  prefix correctly detects bugs with `apron-octagon` (no `LD_LIBRARY_PATH`
  required — APRON is statically linked).
- **64/64 tests passing** — full suite passes in 28 seconds with APRON enabled.
- **Precision benchmark documented** — `APRON_PRECISION_BENCHMARK.md` records
  interval vs apron-octagon comparison across 6 analysis suites (255 tests).

---

## [0.13.0] — 2026-06-14


### 🔬 APRON Abstract Domain Library Support — 64/64 Tests Passing

This release adds optional support for the [APRON](https://github.com/antoinemine/apron)
numerical abstract domain library, enabling 13 additional analysis domains with
stronger relational reasoning capabilities.

### Added

- **APRON integration** — `cmake/FindAPRON.cmake` updated with:
  - `APRON_ITV_LIB` (`libitvMPQ`) added to required libraries (needed by
    `libap_ppl` and `libap_pkgrid` at link time — missing from original find module).
  - `-Wl,--start-group`/`-Wl,--end-group` linker groups to resolve circular
    dependencies between APRON domain libs and `libapron` core (static linking).
  - Domain libraries reordered: consumers (`box`, `oct`, `polka`, `ppl`,
    `pkgrid`) before `libapron` core.

- **13 APRON abstract domains** — all verified functional:
  - Base: `apron-interval`, `apron-octagon`, `apron-polka-polyhedra`,
    `apron-polka-linear-equalities`, `apron-ppl-polyhedra`,
    `apron-ppl-linear-congruences`, `apron-pkgrid-polyhedra-lin-cong`
  - Variable-packing variants: `var-pack-apron-{octagon,polka-polyhedra,
    polka-linear-equalities,ppl-polyhedra,ppl-linear-congruences,
    pkgrid-polyhedra-lin-cong}`

- **APRON regression test suite** (`analyzer/test/regression/apron/`) — 4 tests:
  - Safe loop with `apron-octagon` (BOA smoke test)
  - Buffer overflow detection with `apron-octagon` (BOA error test)
  - Interprocedural safe loop with `var-pack-apron-octagon`
  - Null dereference detection with `apron-octagon` (nullity checker)

- **`--domain` CLI override** in `libruntest.py` — run the full regression suite
  with any abstract domain via `python3 runtest --domain apron-octagon`.

- **Precision benchmark** — `apron-octagon` passes 251/255 regression tests
  with 138 `PASS_IMPROVE` results (stronger precision than default `interval`).
  4 false positives on tests designed for `interval-congruence`/`gauge` domains.

### Build notes

- APRON is an **optional** dependency. NIKOS builds without it; APRON support
  activates automatically when `-DAPRON_ROOT=/path/to/apron/install` is passed.
- APRON must be built from source (`antoinemine/apron`) — not available via apt.
- Build with C API only (`HAS_OCAML=`; `HAS_OCAMLOPT=`). OCaml bindings
  require `mlgmpidl` and are not needed by NIKOS.
- APRON libs are statically linked — no `LD_LIBRARY_PATH` required.

### Test results

- **64/64** tests passing (59 existing + 4 APRON core unit tests + 1 APRON
  regression suite)

---

## [0.12.0] — 2026-06-14


### 🎉 Production Ready — 59/59 Tests Passing

This release marks NIKOS as production-ready with full LLVM 20 support.

### Added

- **`--opt=custom` mode restored** for `ikos-pp` — supports `--lower-select`,
  `--lower-cst-expr`, `--remove-printf-calls`, `--remove-unreachable-blocks`,
  `--name-values`, and `--mark-internal-inline` flags.
- **Opaque pointer tolerance mode** in test framework — `OPAQUE_PTR_TOLERANCE`
  in `libruntest.py` treats precision losses from opaque pointers as
  `PASS_IMPROVE` instead of `FAIL`.

### Fixed

- **VLA (variable-length array) crash** — `translate_array_di_type` in
  `type.cpp` now handles `DISubrange` with `DIVariable` count (LLVM 20 VLA
  representation) instead of crashing in `translate_basic_di_type`.
- **Array allocation type mismatch** — `infer_type_from_dbg` in `function.cpp`
  now always catches `TypeDebugInfoMismatch` for dynamic allocas, regardless
  of `_allow_debug_info_mismatch` setting.
- **Opaque pointer type checker** — `ar::TypeVerifier` relaxed to allow:
  - Bitcasts involving opaque types
  - Load/store through opaque pointers
  - `{opaque*, si32}` as exception structure (LLVM 20 `__cxa_throw`)
  - Implicit bitcasts with opaque types in function call parameters
- **Import test patterns** — regenerated 171 `.ll` FileCheck patterns across
  `no_optimization`, `basic_optimization`, and `aggressive_optimization`
  for LLVM 20's opaque pointer AR output. Reduced import skip lists from
  9/6/3 to 3/3/2 files.

### Changed

- **Test harness** — `libruntest.py` catches `CalledProcessError` from
  analyzer crashes, reporting them as `FAIL` instead of aborting the suite.

### Removed

- Stale `.orig`/`.rej` patch artifacts, debug GDB scripts, and build logs.

## [0.6.1] — 2026-06-14

### Fixed

- **`llvm::Optional` → `std::optional` migration** across all 27 checker
  files (includes and implementations). `llvm::Optional` was removed in
  LLVM 17; the replacement is `std::optional` / `std::nullopt` from C++17.
  Affected files: `checker.hpp`, `buffer_overflow`, `checker`, `null_dereference`,
  `concurrent_inliner`, `double_free`, and all remaining checker headers/sources.

- **Missing transitively-included headers** in `analyzer/src/database/table/operands.cpp`.
  LLVM 20 tightened header hygiene — four headers previously pulled in
  transitively are now required explicitly:
  - `llvm/ADT/SmallString.h` — for `APInt::toString(SmallVectorImpl<char>&)`
  - `llvm/BinaryFormat/Dwarf.h` — for `dwarf::DW_TAG_*` constants
  - `llvm/IR/GetElementPtrTypeIterator.h` — for `gep_type_begin`/`gep_type_end`
  - `llvm/IR/GlobalAlias.h` — for complete `GlobalAlias` type (enables `.getAliasee()`)

  Without these, `ikos-analyzer` failed to compile with errors like
  `no member named 'DW_TAG_typedef' in namespace 'llvm::dwarf'` and
  `member access into incomplete type 'llvm::GlobalAlias'`.

## [0.6.0] — 2026-06-14

### Added

- `ikos-pp` fully ported to LLVM 20 with hybrid legacy/new PassManager.
- `preprocess_module()` in `NikosBridge::import()` — runs 5 mandatory
  lowering passes before AR import (LowerSwitch, LowerAtomic, LowerCstExpr,
  LowerSelect, UnifyFunctionExitNodes).
- `libikos-pp.a` now linked into Nitpick for IKOS-specific pass access.

### Changed

- 7 LLVM passes migrated from removed legacy API to new PassInfoMixin:
  `GlobalDCEPass`, `GlobalOptPass`, `InternalizePass`, `JumpThreadingPass`,
  `SCCPPass`, `LoopDeletionPass`, `UnifyFunctionExitNodesPass`.
- Removed 11 dead `initializeXxxPass()` calls for passes removed in LLVM 20.
- `frontend/llvm/CMakeLists.txt`: added `passes` LLVM component for PassBuilder.

### Fixed

- `NikosBridge::import()` now handles `select`, `switch`, atomic, and constant
  expression instructions (previously threw `ImportError`).
- `ikos-pp` binary now compiles on LLVM 20 (was broken with 25+ errors).

## [0.5.1] — 2026-06-14

### Added

- `NikosWarningPromoter` class for automated Z3 SMT query generation from IKOS warnings.
- SMT-LIB2 query templates for 7 check kinds:
  - Null dereference (`QF_BV`)
  - Division by zero (`QF_LIA`)
  - Buffer overflow (`QF_LIA`)
  - Signed integer overflow (`QF_BV`)
  - Unsigned integer overflow (`QF_BV`)
  - Shift count (`QF_LIA`)
  - Pointer overflow (`QF_BV`)
- `WarningPromotion` outcomes: `PromotedToError` (SAT), `DismissedAsSafe` (UNSAT), `Unchanged`, `NotApplicable`.
- `WarningPromotionReport` with text formatting.
- Direct use of Z3 C API (`Z3_eval_smtlib2_string`).

## [0.5.0] — 2026-06-14

### Added

- `NikosAnalysisRunner`: subprocess driver for `ikos-analyzer`.
- `NikosCrossValidator`: matches IKOS checks against Z3 findings by location and category.
- `CrossValidationReport` with verdicts: `Confirmed`, `Dismissed`, `IkosOnly`, `Z3Only`, `Inconclusive`.
- `NikosBridge::analyze()` and `can_analyze()` methods.
- Binary discovery: `NIKOS_ANALYZER_PATH` env var, sibling directory, `PATH`.

## [0.4.2] — 2026-06-14

### Added

- `NikosReport` with per-function metrics (name, params, locals, basic blocks, statements).
- `to_text()` formatter with Unicode box-drawing table.
- `to_json()` formatter for SARIF/tooling integration.
- `NikosBridge::report()` method.

## [0.4.1] — 2026-06-14

### Added

- Verified `NikosBridge::import(llvm::Module&)` fulfills in-memory ingestion requirement.

## [0.4.0] — 2026-06-13

### Added

- `NikosBridge` class with PIMPL pattern for clean API boundary.
- `NikosBridge::import(llvm::Module&)` for LLVM module to AR bundle translation.
- Linked `libikos-ar.a` and `libikos-llvm-to-ar.a` into Nitpick's `npkc` binary.
- 82 NIKOS symbols initially linked.

## [0.3.1] — 2026-06-13

### Fixed

- Type translation for LLVM 20's opaque pointer migration.
- Aggregate type (struct/array) translation.
- Ensured AR types faithfully represent LLVM IR semantics.

## [0.3.0] — 2026-06-13

### Fixed

- Audited all AR (Abstract Representation) factory methods for LLVM 20 compatibility.
- Bundle creation, function signatures, and basic block construction.
- Statement translation (assignments, assertions, calls).

## [0.2.3] — 2026-06-12

### Fixed

- Completed LLVM frontend compilation with LLVM 20.
- `ikos-pp` (preprocessor) and `ikos-import` for opaque pointers.
- LLVM PassManager API changes.

## [0.2.2] — 2026-06-12

### Fixed

- All compilation errors in the AR (Abstract Representation) library.
- Type system updated for LLVM 20 changes.

## [0.2.1] — 2026-06-12

### Fixed

- All compilation errors in the core abstract interpretation library.
- Abstract domains and fixpoint iterators updated for C++17.

## [0.2.0] — 2026-06-12

### Added

- Forked from [NASA-SW-VnV/ikos](https://github.com/NASA-SW-VnV/ikos) (LLVM 14).
- Initial compilation fixes across all components.

### Changed

- Updated CMake to find LLVM 20.
- C++ standard from C++14 to C++17.

[2.3.1]: https://github.com/alternative-intelligence-cp/nikos/compare/v2.3.0...v2.3.1
[2.3.0]: https://github.com/alternative-intelligence-cp/nikos/compare/v2.2.0...v2.3.0
[2.2.0]: https://github.com/alternative-intelligence-cp/nikos/compare/v2.1.0...v2.2.0
[2.1.0]: https://github.com/alternative-intelligence-cp/nikos/compare/v1.0.1...v2.1.0
[0.12.0]: https://github.com/alternative-intelligence-cp/nikos/compare/v0.6.1...v0.12.0
[0.6.1]: https://github.com/alternative-intelligence-cp/nikos/compare/v0.6.0...v0.6.1
[0.6.0]: https://github.com/alternative-intelligence-cp/nikos/compare/v0.5.1...v0.6.0
[0.5.1]: https://github.com/alternative-intelligence-cp/nikos/compare/v0.5.0...v0.5.1
[0.5.0]: https://github.com/alternative-intelligence-cp/nikos/compare/v0.4.2...v0.5.0
[0.4.2]: https://github.com/alternative-intelligence-cp/nikos/compare/v0.4.1...v0.4.2
[0.4.1]: https://github.com/alternative-intelligence-cp/nikos/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/alternative-intelligence-cp/nikos/compare/v0.3.1...v0.4.0
[0.3.1]: https://github.com/alternative-intelligence-cp/nikos/compare/v0.3.0...v0.3.1
[0.3.0]: https://github.com/alternative-intelligence-cp/nikos/compare/v0.2.3...v0.3.0
[0.2.3]: https://github.com/alternative-intelligence-cp/nikos/compare/v0.2.2...v0.2.3
[0.2.2]: https://github.com/alternative-intelligence-cp/nikos/compare/v0.2.1...v0.2.2
[0.2.1]: https://github.com/alternative-intelligence-cp/nikos/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/alternative-intelligence-cp/nikos/releases/tag/v0.2.0

