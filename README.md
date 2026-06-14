# NIKOS

**New Inference Kernel for Open Static Analyzers**

A modernized fork of [NASA's IKOS](https://github.com/NASA-SW-VnV/ikos), upgraded from LLVM 14 to **LLVM 20**.
Detect and prove the absence of runtime errors in C/C++ using Abstract Interpretation.

![LLVM 20](https://img.shields.io/badge/LLVM-20-blue?style=flat-square&logo=llvm)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square&logo=cplusplus)
![Tests](https://img.shields.io/badge/tests-64%2F64%20passing-brightgreen?style=flat-square)
![APRON](https://img.shields.io/badge/APRON-optional-blue?style=flat-square)
![License: NOSA 1.3](https://img.shields.io/badge/License-NOSA%201.3-green?style=flat-square)

---

## Table of Contents

- [Why NIKOS?](#why-nikos)
- [Architecture](#architecture)
- [Checkers](#checkers)
- [Prerequisites](#prerequisites)
- [Building from Source](#building-from-source)
- [APRON Support (Optional)](#apron-support-optional)
- [Usage](#usage)
- [Library Integration](#library-integration)
- [Releases](#releases)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgements](#acknowledgements)

## Why NIKOS?

[IKOS](https://github.com/NASA-SW-VnV/ikos) is a powerful static analyzer developed by NASA, with over 3,200 stars on GitHub — but it is pinned to **LLVM 14.0.x** and has not been updated since 2022. Modern LLVM toolchains (17+) introduced breaking changes that make upstream IKOS unbuildable:

| Challenge | What Changed |
|---|---|
| **Opaque pointers** | LLVM removed typed pointers; all `ptr` types are now opaque |
| **Debug info records** | Intrinsic-based debug info replaced by `DbgRecord` |
| **PassBuilder API** | Legacy pass manager removed in favor of the new PM |

NIKOS solves this. If your project uses LLVM 17–20 and you need abstract interpretation, NIKOS is a drop-in path forward.

### Key Changes from Upstream

- **LLVM 14 → LLVM 20** — full opaque pointer support, modern debug info, new PassBuilder
- **C++14 → C++17** — structured bindings, `std::optional`, `if constexpr`
- **Fixed type translation** for LLVM's opaque pointer migration
- **Updated AR factory** for LLVM 20 IR structure changes
- **`llvm::Optional` removed** — all checker headers/sources migrated to `std::optional`/`std::nullopt` (27 files; `llvm::Optional` was dropped in LLVM 17)
- **Explicit header includes** — LLVM 20 tightened transitive-include guarantees; `SmallString.h`, `BinaryFormat/Dwarf.h`, `GetElementPtrTypeIterator.h`, and `GlobalAlias.h` are now explicitly included in `operands.cpp`
- **Dynamic linking support** — `libikos-ar.a` and `libikos-llvm-to-ar.a` are usable as libraries
- **In-memory module ingestion** — analyze LLVM modules without writing bitcode to disk

## Architecture

NIKOS is organized into four layers. Each layer depends only on the ones below it:

```
┌──────────────────────────────────────────────────────────┐
│  analyzer/          Runs checkers on AR, reports bugs    │
│  ikos-analyzer                                           │
├──────────────────────────────────────────────────────────┤
│  frontend/llvm/     Translates LLVM IR → AR              │
│  ikos-pp, ikos-import                                    │
├──────────────────────────────────────────────────────────┤
│  ar/                Abstract Representation (generic     │
│                     assembly language, simpler than IR)   │
├──────────────────────────────────────────────────────────┤
│  core/              Abstract Interpretation framework    │
│                     Domains, fixpoint iterators, CFGs    │
└──────────────────────────────────────────────────────────┘
```

**Data flow:** `C/C++ source` → *clang* → `LLVM IR` → *ikos-pp* → `preprocessed IR` → *ikos-import* → `AR` → *ikos-analyzer* → `results (SQLite / JSON)`

## Checkers

NIKOS ships 16 analysis checkers:

| Checker | ID | Detects |
|---|---|---|
| Buffer Overflow | `boa` | Out-of-bounds array/pointer access |
| Division by Zero | `dbz` | Integer and floating-point division by zero |
| Null Pointer Dereference | `nullity` | Null pointer reads/writes |
| Signed Integer Overflow | `sio` | Signed arithmetic overflow (UB) |
| Unsigned Integer Overflow | `uio` | Unsigned arithmetic wrapping |
| Shift Count | `shc` | Invalid shift amounts |
| Pointer Overflow | `poa` | Pointer arithmetic overflow |
| Uninitialized Variable | `uva` | Use of uninitialized memory |
| Dead Code | `dca` | Unreachable code |
| Double Free | `dfa` | Double-free / use-after-free |
| Soundness | `sound` | Analysis soundness warnings |
| Function Call | `fca` | Invalid function calls |
| Memory | `mem` | General memory safety |
| Pointer Alignment | `pcmp` | Misaligned pointer access |
| Assertion Prover | `prover` | User-supplied assertion verification |
| Debug | `dbg` | Debug checker internals |

## Prerequisites

| Dependency | Minimum Version | Required |
|---|---|---|
| LLVM + Clang | 20 | ✅ |
| CMake | 3.20 | ✅ |
| C++ compiler | GCC ≥ 9 or Clang ≥ 10 | ✅ |
| Boost | 1.71 | ✅ |
| GMP | 6.1 | ✅ |
| SQLite3 | 3.27 | ✅ |
| TBB | 2020 | ✅ |
| Python | 3.8 | ✅ |
| zlib, libedit | (system) | ✅ |
| MPFR | 4.0 | Optional (APRON) |
| PPL | 1.2 | Optional (APRON) |
| APRON | source build | Optional |

## Building from Source

Tested on **Ubuntu 22.04** and **Ubuntu 24.04**.

### 1. Install dependencies

```bash
sudo apt-get update
sudo apt-get install -y cmake libboost-all-dev libgmp-dev \
  libsqlite3-dev libtbb-dev libedit-dev zlib1g-dev python3 \
  llvm-20-dev clang-20 libclang-20-dev
```

### 2. Clone and build

```bash
git clone https://github.com/alternative-intelligence-cp/nikos.git
cd nikos
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$(pwd)/../install \
      -DLLVM_CONFIG_EXECUTABLE=$(which llvm-config-20) ..
make -j$(nproc)
make install
```

The install tree will contain `bin/`, `lib/`, and `include/` directories under `nikos/install/`.

## APRON Support (Optional)

[APRON](https://github.com/antoinemine/apron) is a C library of numerical abstract domains for static analysis. Enabling it adds 13 additional analysis domains with stronger relational reasoning:

| Domain | Flag |
|---|---|
| `apron-interval` | APRON interval (baseline) |
| `apron-octagon` | Octagon domain (x±y≤c constraints) |
| `apron-polka-polyhedra` | Convex polyhedra (maximum precision) |
| `apron-polka-linear-equalities` | Linear equalities only |
| `apron-ppl-polyhedra` | PPL polyhedra |
| `apron-ppl-linear-congruences` | PPL linear congruences |
| `apron-pkgrid-polyhedra-lin-cong` | Product of polyhedra + congruences |
| `var-pack-apron-*` | Variable-packing variants of all above |

### Build APRON from source

```bash
# Install APRON build dependencies
sudo apt-get install -y libmpfr-dev libppl-dev ocaml ocaml-findlib camlidl m4

# Clone and build APRON (C API only, no OCaml bindings)
git clone https://github.com/antoinemine/apron.git
cd apron
cp Makefile.config.model Makefile.config
# Edit Makefile.config:
#   APRON_PREFIX = /path/to/apron/install
#   HAS_PPL = 1
#   HAS_OCAML =
#   HAS_OCAMLOPT =
make -j$(nproc)
make install  # or manually copy headers and .a files
```

### Build NIKOS with APRON

```bash
cmake -DAPRON_ROOT=/path/to/apron/install \
      -DLLVM_CONFIG_EXECUTABLE=$(which llvm-config-20) ..
make -j$(nproc)
```

> **Note:** APRON is not thread-safe. Do not use `-j N` parallelism when running analysis with APRON domains.

### Use an APRON domain

```bash
# Use octagon domain for stronger relational precision
ikos-analyzer -a=boa -d=apron-octagon -entry-points=main example.pp.bc -o out.db

# Use polka-polyhedra for maximum precision (slower)
ikos-analyzer -a=boa -d=apron-polka-polyhedra -entry-points=main example.pp.bc -o out.db
```

## Usage

### Quick start

```bash
# Add NIKOS to your PATH
export PATH=/path/to/nikos/install/bin:$PATH

# Analyze a C file (all checkers, default settings)
ikos example.c
```

### Analyze pre-compiled bitcode

```bash
# Compile to LLVM bitcode
clang-20 -c -emit-llvm -g -O0 example.c -o example.bc

# Run the analyzer directly
ikos-analyzer --display-checks=all example.bc
```

### Select specific checkers

```bash
ikos --analyses=boa,nullity,dbz example.c
```

Results are stored in an SQLite database by default. Use `ikos-report` to view them, or pass `--display-checks=all` for inline output.

## Library Integration

NIKOS can be embedded as a **C++ library** in other LLVM-based tools. Link against the static archives:

- `libikos-ar.a` — Abstract Representation layer
- `libikos-llvm-to-ar.a` — LLVM IR → AR translation

### CMake example

```cmake
set(NIKOS_DIR "${CMAKE_SOURCE_DIR}/../nikos")

target_include_directories(mytool PRIVATE
  ${NIKOS_DIR}/core/include
  ${NIKOS_DIR}/ar/include
  ${NIKOS_DIR}/frontend/llvm/include
)

target_link_libraries(mytool
  ${NIKOS_DIR}/build/ar/libikos-ar.a
  ${NIKOS_DIR}/build/frontend/llvm/libikos-llvm-to-ar.a
)
```

**Production example:** [Nitpick](https://github.com/alternative-intelligence-cp/nitpick) uses NIKOS for cross-validation between abstract interpretation and SMT solving.

## Releases

| Tag | Milestone |
|---|---|
| `v0.13.1` | **0.13 Series Final** — install verification, benchmark docs, series close |
| `v0.13.0` | **APRON Support** — 13 relational abstract domains (64/64 tests passing) |
| `v0.12.0` | **Production Ready** — 59/59 tests passing, opaque pointer type system fixes, VLA support, `--opt=custom` restored |
| `v0.6.1` | Checker `llvm::Optional` migration; `operands.cpp` header hygiene |
| `v0.6.0` | `ikos-pp` LLVM 20 Port (hybrid PassManager) |
| `v0.5.1` | Automated Z3 SMT Generation |
| `v0.5.0` | Cross-Validation (IKOS vs Z3) |
| `v0.4.2` | TUI/GUI Reporting |
| `v0.4.1` | In-memory Module Ingestion |
| `v0.4.0` | Dynamic Linking |
| `v0.3.1` | Type Fidelity (opaque pointers) |
| `v0.3.0` | AR Factory Audit |
| `v0.2.3` | Frontend LLVM Compilation (LLVM 20) |

## Troubleshooting

Build failures, LLVM version mismatches, and common runtime issues are covered in **[TROUBLESHOOTING.md](TROUBLESHOOTING.md)**.

### Quick LLVM 20 compile-error reference

| Error message | Fix |
|---|---|
| `no member named 'DW_TAG_typedef' in namespace 'llvm::dwarf'` | Add `#include <llvm/BinaryFormat/Dwarf.h>` |
| `member access into incomplete type 'llvm::GlobalAlias'` | Add `#include <llvm/IR/GlobalAlias.h>` |
| `no member named 'toString'` on `APInt` | Add `#include <llvm/ADT/SmallString.h>` |
| `use of undeclared 'gep_type_begin'` | Add `#include <llvm/IR/GetElementPtrTypeIterator.h>` |
| `'llvm::Optional' is not a member of 'llvm'` | Replace with `std::optional`; `llvm::None` → `std::nullopt` |
| `initializeXxxPass` linker errors | Remove calls — these passes were deleted in LLVM 17+ |

## Contributing

Contributions are welcome! Whether it's bug reports, new checkers, documentation, or LLVM compatibility patches — feel free to open an issue or pull request.

Please see **[CONTRIBUTING.md](CONTRIBUTING.md)** for guidelines, or check **[CONTRIBUTORS.md](CONTRIBUTORS.md)** to see who has contributed so far.

## License

NIKOS is released under the **NASA Open Source Agreement (NOSA) version 1.3**, inherited from upstream IKOS. See [LICENSE.txt](LICENSE.txt) for the full text.

## Acknowledgements

NIKOS is built on the foundation of [IKOS](https://github.com/NASA-SW-VnV/ikos), originally developed by the **NASA Ames Research Center** Software Verification & Validation team. We are grateful for their pioneering work in applying Abstract Interpretation to real-world software safety.
