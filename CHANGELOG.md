# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

**NIKOS** is a fork of [NASA's IKOS](https://github.com/NASA-SW-VnV/ikos), ported from LLVM 14 to LLVM 20 and integrated into the Nitpick static analysis toolchain.

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

[0.5.1]: https://github.com/user/nikos/compare/v0.5.0...v0.5.1
[0.5.0]: https://github.com/user/nikos/compare/v0.4.2...v0.5.0
[0.4.2]: https://github.com/user/nikos/compare/v0.4.1...v0.4.2
[0.4.1]: https://github.com/user/nikos/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/user/nikos/compare/v0.3.1...v0.4.0
[0.3.1]: https://github.com/user/nikos/compare/v0.3.0...v0.3.1
[0.3.0]: https://github.com/user/nikos/compare/v0.2.3...v0.3.0
[0.2.3]: https://github.com/user/nikos/compare/v0.2.2...v0.2.3
[0.2.2]: https://github.com/user/nikos/compare/v0.2.1...v0.2.2
[0.2.1]: https://github.com/user/nikos/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/user/nikos/releases/tag/v0.2.0
