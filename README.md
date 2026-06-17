# NIKOS

**Nitpick Inference Kernel for Open Static Analyzers**

A modernized fork of [NASA's IKOS](https://github.com/NASA-SW-VnV/ikos), upgraded from LLVM 14 to **LLVM 20**.
Detect and prove the absence of runtime errors in C/C++ using Abstract Interpretation.

![Version](https://img.shields.io/badge/version-2.3.1.1-purple?style=flat-square)
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
- [Installation](#installation)
- [Releases](#releases)
- [Troubleshooting](#troubleshooting)
- [Working with the Test Suite](#working-with-the-test-suite)
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

## Installation

NIKOS v2.0.0 provides multiple installation methods. 

> [!NOTE]
> **Version Guidance**
> - **Version 1.0.1:** If you are simply looking for a direct, modernized drop-in replacement for the original NASA IKOS (updated for LLVM 20, opaque pointers, modern C++), use the **v1.0.1** release. We will maintain the 1.0 series with critical bug fixes only, keeping it clean and simple for legacy users wanting no additional bells and whistles.
> - **Version 2.0.0:** If you want all of our powerful new additions—Taint Analysis, Use-After-Move Detection, Advanced Concurrency Modeling (std::thread), Web APIs, and more—use the **v2.0.0** release. This is the latest release containing all the new goodies built for the Nitpick ecosystem.

### One-line Install Script (Ubuntu 22.04 / 24.04)

```bash
bash <(curl -sSf https://raw.githubusercontent.com/alternative-intelligence-cp/nikos/main/script/install.sh)
```

With APRON support:
```bash
bash <(curl -sSf .../install.sh) --with-apron
```

### Native Packages (.deb / .rpm)

```bash
# Ubuntu / Debian
wget https://github.com/alternative-intelligence-cp/nikos/releases/download/v2.0.0/nikos-2.0.0-Linux.deb
sudo dpkg -i nikos-2.0.0-Linux.deb || sudo apt-get install -f -y

# Fedora / RHEL
wget https://github.com/alternative-intelligence-cp/nikos/releases/download/v2.0.0/nikos-2.0.0-Linux.rpm
sudo dnf install -y nikos-2.0.0-Linux.rpm
```

### Docker

```bash
docker build -t nikos:1.0.0 .
docker run --rm -v $(pwd):/work nikos:1.0.0 -a=boa,nullity -d=interval \
  -entry-points=main -proc=inter /work/my_program.pp.bc -o /work/results.db
```
See [doc/DOCKER.md](doc/DOCKER.md) for full usage.

### From Source

See [doc/install/1.0/UBUNTU_22.04.md](doc/install/1.0/UBUNTU_22.04.md) or [doc/install/1.0/UBUNTU_24.04.md](doc/install/1.0/UBUNTU_24.04.md) for step-by-step guides.

## Releases

| Tag | Milestone |
|---|---|
| **`v2.3.1.1`** | **🔧 Patch Release** — CI compatibility fixes for Linux and macOS (pip _internal error & taint_config.json path) |
| `v2.3.1` | 🔧 Patch Release — Complete LLVM 20 regression test suite fixed (all 162 tests pass); `regen_checks.py` tool; LLVM 14→20 AR changes guide |
| `v2.3.0` | 🔒 Security & Taint Analysis Expansion — 14/14 taint tests, network/IO sources, POSIX profile |
| `v2.2.0` | 🧵 Concurrency Checker — Data race & deadlock detection (`pthread_mutex`) |
| `v2.1.0` | 🧹 Use-After-Free Checker Enhancements — `USE_AFTER_FREE`, `USE_AFTER_RETURN`, `USE_AFTER_MOVE` |
| `v2.0.1` | 🔧 CI Compatibility — macOS (AppleClang 17) and Linux build fixes |
| **`v2.0.0`** | **🚀 Version 2.0 Release** — Taint Analysis, Concurrency modeling (`std::thread`, `pthread_once`), Unified API |
| **`v1.0.1`** | **🔧 Patch Release** — CMake 4.x, Boost 1.90, FindPythonInterp, AppleClang 17 fixes |
| **`v1.0.0`** | **🚀 Official Production Release** — install.sh, .deb, Docker, RPM, Flatpak, full docs (64/64 tests) |
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

## Working with the Test Suite

The regression tests in `frontend/llvm/test/regression/import/` validate the AR output of `ikos-import` using LLVM's FileCheck tool. If you are adding tests, updating CHECK lines after an LLVM upgrade, or debugging CI failures, read:

> **[doc/LLVM20_AR_CHANGES.md](doc/LLVM20_AR_CHANGES.md)** — Documents every way the AR output changed between LLVM 14 and LLVM 20: concrete alloca types, struct layout resolution, bitcast elimination rules, SSA renumbering, and more. Also covers how to run the suite locally with the correct LLVM version and how to auto-regenerate CHECK lines.

Key things to know before touching test files:

- The system `ikos-import` may be built against a **different LLVM version** than CI. Always verify with `ikos-import --version`.
- Use `script/regen_checks.py` to regenerate CHECK lines from actual `ikos-import` output instead of editing by hand:
  ```bash
  python3 script/regen_checks.py --batch --failing-only \
    frontend/llvm/test/regression/import/no_optimization/
  ```
- Run the full suite locally before pushing:
  ```bash
  cd frontend/llvm/test/regression/import/no_optimization
  bash runtest --ikos-import /path/to/ikos-import --file-check /path/to/FileCheck
  ```

## Contributing

Contributions are welcome! Whether it's bug reports, new checkers, documentation, or LLVM compatibility patches — feel free to open an issue or pull request.

Please see **[CONTRIBUTING.md](CONTRIBUTING.md)** for guidelines, or check **[CONTRIBUTORS.md](CONTRIBUTORS.md)** to see who has contributed so far.

## License

NIKOS is released under the **NASA Open Source Agreement (NOSA) version 1.3**, inherited from upstream IKOS. See [LICENSE.txt](LICENSE.txt) for the full text.

## Acknowledgements

NIKOS is built on the foundation of [IKOS](https://github.com/NASA-SW-VnV/ikos), originally developed by the **NASA Ames Research Center** Software Verification & Validation team. We are grateful for their pioneering work in applying Abstract Interpretation to real-world software safety.
