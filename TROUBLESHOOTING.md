# Troubleshooting

This document covers common issues with NIKOS and how to resolve them.

**Contact:** [GitHub Issues](https://github.com/alternative-intelligence-cp/nikos/issues)

---

## Build Issues

### "Could NOT find LLVM" while running cmake

CMake could not find LLVM.

First, install LLVM 20:
```bash
wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && sudo ./llvm.sh 20
```

If the error persists, give cmake the full path:
```bash
cmake -DLLVM_CONFIG_EXECUTABLE=/usr/bin/llvm-config-20 ..
```

### "Could NOT find Clang" while running cmake

Install clang-20 and pass its path explicitly:
```bash
sudo apt-get install clang-20
cmake -DCLANG_EXECUTABLE=/usr/bin/clang-20 ..
```

### "/usr/bin/ld: cannot find -lLLVMCore"

Your LLVM was built as a single shared library (`libLLVM.so`). Fix:
```bash
cmake -DBUILD_SHARED_LIBS=ON -DIKOS_LINK_LLVM_DYLIB=ON ..
```

### "Two passes with the same argument (-domtree) attempted to be registered!"

You are mixing shared and static LLVM libraries. Fix:
```bash
cmake -DBUILD_SHARED_LIBS=ON -DIKOS_LINK_LLVM_DYLIB=ON ..
```

---

## LLVM 20-Specific Build Errors

These errors appear when building against LLVM 20 from a stale checkout.
All of these are already fixed in NIKOS v1.0.0.

| Error | Fix |
|---|---|
| `no member named 'DW_TAG_typedef' in namespace 'llvm::dwarf'` | Add `#include <llvm/BinaryFormat/Dwarf.h>` |
| `member access into incomplete type 'llvm::GlobalAlias'` | Add `#include <llvm/IR/GlobalAlias.h>` |
| `no member named 'toString'` on `APInt` | Add `#include <llvm/ADT/SmallString.h>` |
| `use of undeclared 'gep_type_begin'` | Add `#include <llvm/IR/GetElementPtrTypeIterator.h>` |
| `'llvm::Optional' is not a member of 'llvm'` | Replace with `std::optional`; `llvm::None` → `std::nullopt` |
| `initializeXxxPass` linker errors | Remove calls — legacy passes were deleted in LLVM 17+ |
| Opaque pointer type assertion failures | These are handled by `OPAQUE_PTR_TOLERANCE` in the test runner |

---

## APRON Build Issues

### "memory access violation at address: 0x00000088"

This is a bug in system-provided APRON packages. Always build APRON from source:
```bash
git clone https://github.com/antoinemine/apron.git
```
See `doc/install/1.0/APRON.md` for the full build guide.

### APRON linker errors ("undefined reference to `ap_interval_alloc`")

The APRON libraries require a specific link order and the `libitvMPQ` archive.
NIKOS's `cmake/FindAPRON.cmake` handles this automatically. Ensure you pass:
```bash
cmake -DAPRON_ROOT=/path/to/apron/install ..
```

### ikos-analyzer crashes when using APRON domains with multiple threads

APRON is **not thread-safe**. Never use APRON domains with the `-j N` parallel
flag. For analysis, run single-threaded.

---

## Installation Issues

### "Could not find ikos python module" while running ikos

The `ikos` front-end wrapper needs the Python module on the path:
```bash
export PYTHONPATH=/path/to/nikos/install/lib/python*/ikos
```

Or add it permanently to your shell profile.

---

## Analysis Issues

### Exited with signal SIGKILL (out of memory)

NIKOS ran out of memory. Options:

1. Disable the fixpoint cache (reduces memory at the cost of speed):
   ```bash
   ikos-analyzer --no-fixpoint-cache ...
   ```
2. Use intraprocedural analysis (no cross-function summary):
   ```bash
   ikos-analyzer --proc=intra ...
   ```
3. Analyze individual translation units separately.

### False positives from external library functions

NIKOS treats unknown library functions as returning unconstrained values,
which can cause spurious warnings at call sites. Limit the analysis scope:
```bash
ikos-analyzer -a=boa,nullity --entry-points=my_func ...
```

### Source Code Fortification (`__memset_chk`, `__memcpy_chk`)

With `-D_FORTIFY_SOURCE=2`, glibc replaces `memset`/`memcpy` with
`__memset_chk`/`__memcpy_chk`. NIKOS treats these as unknown functions.
Fix by disabling fortification at compile time:
```bash
clang-20 -D_FORTIFY_SOURCE=0 -c -emit-llvm -g -O0 myfile.c -o myfile.bc
```

### Analyzing multi-threaded code

NIKOS does not model multi-threaded execution. Concurrent programs will
produce unsound results. This is a known limitation of single-threaded
abstract interpretation.
