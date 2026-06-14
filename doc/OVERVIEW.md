Overview of the Source Code
===========================

NIKOS is a modernized fork of [NASA's IKOS](https://github.com/NASA-SW-VnV/ikos), upgraded from LLVM 14 to LLVM 20.

The following illustrates the content of the root directory:

```
.
├── CMakeLists.txt
├── CHANGELOG.md
├── CONTRIBUTING.md
├── CONTRIBUTORS.md
├── LICENSE.pdf
├── LICENSE.txt
├── README.md
├── TROUBLESHOOTING.md
├── analyzer
├── ar
├── cmake
├── core
├── doc
├── frontend
├── script
└── test
```

* [CMakeLists.txt](../CMakeLists.txt) is the root CMake file. Configures `project(nikos)` with C++17.

* [CHANGELOG.md](../CHANGELOG.md) contains the change log for all releases since the LLVM 20 port.

* [CONTRIBUTING.md](../CONTRIBUTING.md) contains guidelines for contributing to NIKOS.

* [CONTRIBUTORS.md](../CONTRIBUTORS.md) lists the original IKOS contributors.

* [LICENSE.pdf](../LICENSE.pdf) contains the NOSA 1.3 license.

* [TROUBLESHOOTING.md](../TROUBLESHOOTING.md) contains solutions for common issues with NIKOS.

* [analyzer](../analyzer) contains the implementation of various analyses for specific defect detections. These analyses are implemented on the Abstract Representation and use the fixpoint iterator and abstract domains to perform analysis.

* [ar](../ar) contains the implementation of the Abstract Representation, a generic assembly language.

* [cmake](../cmake) contains CMake modules to find dependencies (LLVM, Boost, GMP, SQLite, TBB, etc.).

* [core](../core) contains the implementation of the theory of Abstract Interpretation, which includes the abstract domains, the fixpoint iterator, and various algorithms that support the implementation.

* [doc](../doc) contains documentation including this overview, coding standards, and installation guides.

* [frontend/llvm](../frontend/llvm) contains the implementation of the translation from LLVM IR to AR. This is where the LLVM 20 porting work is concentrated.

* [script](../script) contains the [bootstrap](../script/bootstrap) script for a rootless installation.

* [test/install](../test/install) contains tests for the installation of NIKOS on different platforms.

## Architecture

```
                 C/C++ Source
                      │
                      ▼
              clang-20 (compile)
                      │
                      ▼
              LLVM 20 Bitcode (.bc)
                      │
                      ▼
              ikos-pp (preprocess)
                      │
                      ▼
            Optimized LLVM Bitcode
                      │
                      ▼
        frontend/llvm (LLVM IR → AR)
                      │
                      ▼
        Abstract Representation (AR)
                      │
                      ▼
         analyzer (abstract interp)
                      │
                      ▼
              Analysis Results
```
