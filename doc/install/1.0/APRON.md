# NIKOS v1.0.0 — APRON Numerical Domains Build Guide

> **Applies to:** Ubuntu 22.04 LTS, Ubuntu 24.04 LTS  
> **APRON source:** https://github.com/antoinemine/apron  
> **Last updated:** 2026-06-14

---

## Table of Contents

1. [What is APRON?](#1-what-is-apron)
2. [When to Install APRON](#2-when-to-install-apron)
3. [Prerequisites](#3-prerequisites)
4. [Clone APRON](#4-clone-apron)
5. [Configure Makefile.config](#5-configure-makefileconfig)
6. [Build APRON](#6-build-apron)
7. [Install APRON](#7-install-apron)
8. [Rebuild NIKOS with APRON Support](#8-rebuild-nikos-with-apron-support)
9. [Testing APRON Domains](#9-testing-apron-domains)
10. [Known Issues and Limitations](#10-known-issues-and-limitations)
11. [Troubleshooting](#11-troubleshooting)
12. [Available Domains Reference](#12-available-domains-reference)

---

## 1. What is APRON?

[APRON](https://antoinemine.github.io/Apron/doc/) (Abstract PRiority ON-intervals) is an open-source C library implementing a unified API for **numerical abstract domains** used in static program analysis. It was developed by Antoine Miné and others as part of the Astrée analyzer project.

APRON provides the following concrete domain implementations, each offering a different trade-off between precision and computational cost:

| Domain | Description | Precision | Cost |
|--------|-------------|-----------|------|
| Box (Interval) | Per-variable intervals | Low | Very fast |
| Octagon | ±x ± y ≤ c constraints | Medium | Moderate |
| New Polka (Convex Polyhedra) | Linear inequalities | High | Expensive |
| PPL Polyhedra | Parma Polyhedra Library backend | High | Expensive |
| PPL Grid | Linear congruences | Medium | Moderate |
| PkGrid | Polyhedral + grid product | High | Very expensive |

When integrated with NIKOS, APRON unlocks **13 additional abstract domains** accessible via the `-d=apron-*` family of command-line flags, enabling significantly more precise analysis at the cost of longer analysis time.

---

## 2. When to Install APRON

Install APRON when:

- You need **high-precision analysis** of numerical invariants (e.g., proving absence of buffer overflows in code with complex loop bounds)
- The built-in `interval` and `congruence` domains produce too many false positives on your codebase
- You are analyzing safety-critical or security-critical code where precision outweighs analysis time
- You specifically need the **octagon** or **convex polyhedra** domains

Skip APRON when:

- You only need basic null-pointer or type-safety checks (the built-in domains are sufficient)
- You are doing a quick exploratory scan of a large codebase (APRON domains are slower)
- You are building in a CI/CD pipeline where build reproducibility and simplicity matter more than precision

---

## 3. Prerequisites

### 3.1 Install Required System Packages

```bash
sudo apt-get update
sudo apt-get install -y \
    libmpfr-dev \
    libppl-dev \
    m4 \
    libgmp-dev
```

| Package | Purpose |
|---------|---------|
| `libmpfr-dev` | GNU MPFR multi-precision floating-point library (required by APRON's floating-point domains) |
| `libppl-dev` | Parma Polyhedra Library (enables PPL-backed domains: `ppl_poly`, `ppl_grid`, `pkgrid`) |
| `m4` | Macro processor (required by APRON's build system) |
| `libgmp-dev` | GNU Multiple Precision Arithmetic (required by APRON and PPL) |

> **Note:** `libgmp-dev` was already installed as part of the main NIKOS dependency installation. It is listed here for completeness.

### 3.2 OCaml Is NOT Required

APRON has optional OCaml bindings, but NIKOS uses the **C API only**. You do **not** need to install:

- `ocaml`
- `camlidl`
- `ocaml-findlib`

Do not install these packages. If OCaml is already present on your system, APRON's build system may attempt to build the OCaml bindings and fail. The [Makefile.config configuration](#5-configure-makefileconfig) section explains how to explicitly disable OCaml to avoid this.

---

## 4. Clone APRON

Clone the official APRON repository from GitHub:

```bash
git clone https://github.com/antoinemine/apron.git
cd apron
```

> The APRON repository does not publish versioned release tarballs for the C library. Build from the `master` branch, which is stable for C-API usage.

---

## 5. Configure Makefile.config

APRON uses a hand-written `Makefile`-based build system. Configuration is done by editing `Makefile.config`.

### 5.1 Copy the Template

```bash
cp Makefile.config.model Makefile.config
```

### 5.2 Edit Makefile.config

Open `Makefile.config` in your editor and set the following variables. Lines not mentioned here can be left at their default values.

```bash
# --- Required settings ---

# Installation prefix for APRON headers and libraries.
# Choose a path you have write access to, e.g. a local directory.
APRON_PREFIX = /path/to/apron/install

# --- PPL settings ---

# Enable PPL support (gives you ppl_poly, ppl_grid, pkgrid domains)
HAS_PPL = 1

# Set to 0 if pkg-config cannot locate PPL (common on Ubuntu)
# APRON will link against PPL by path instead of using pkg-config.
HAS_PPL_PKG_CONFIG = 0

# --- OCaml settings (MUST be disabled for C-only build) ---

# Leave all three of these BLANK (no value after the =)
HAS_OCAML =
HAS_OCAMLOPT =
HAS_OCAMLFIND =
```

> **Why `HAS_PPL_PKG_CONFIG = 0`?**
>
> The `libppl-dev` package on Ubuntu does not always install a `.pc` file that pkg-config can find. Setting `HAS_PPL_PKG_CONFIG = 0` forces APRON to link PPL via its known library path (`-lppl`) rather than querying pkg-config. If you know your PPL installation has a working `.pc` file, you may set this to `1`.

### 5.3 Verify APRON_PREFIX

Choose a path that:
- Does not require root privileges (to keep the build user-local)
- Does not conflict with system paths

**Example:**

```
APRON_PREFIX = /home/user/apron-install
```

or alongside the NIKOS install tree:

```
APRON_PREFIX = /home/user/nikos/apron/install
```

Record this path — you will need it when rebuilding NIKOS.

---

## 6. Build APRON

### 6.1 Run Make

```bash
make -j$(nproc)
```

This builds all enabled components: the core `apron` library, `box`, `octagons`, `newpolka`, and optionally `ppl` and `products`.

### 6.2 About OCaml-Related Errors

If OCaml is installed on your system (even partially), you may see errors like:

```
ocamlfind: Command not found
make[2]: *** [mlapronidl.cma] Error 127
make[1]: *** [mlapronidl] Error 2
```

**These errors are safe to ignore**, as long as the C library targets succeeded. Look for successful output such as:

```
ar rcs libapron.a ...
ar rcs libboxD.a ...
ar rcs libboxMPQ.a ...
ar rcs liboctD.a ...
ar rcs liboctMPQ.a ...
ar rcs libpolkaMPQ.a ...
ar rcs libpolkaRll.a ...
ar rcs libap_ppl.a ...
ar rcs libap_pkgrid.a ...
```

If you see those `.a` files being created, the C build succeeded. The OCaml failures do not affect NIKOS.

To suppress OCaml targets and avoid the errors entirely, ensure `HAS_OCAML`, `HAS_OCAMLOPT`, and `HAS_OCAMLFIND` are blank in `Makefile.config` as instructed in [Section 5.2](#52-edit-makefileconfig).

---

## 7. Install APRON

### 7.1 Try the Standard Install First

```bash
make install
```

If this succeeds and populates `$APRON_PREFIX/include/` and `$APRON_PREFIX/lib/`, you are done — skip to [Section 8](#8-rebuild-nikos-with-apron-support).

### 7.2 Manual Installation (Fallback)

`make install` frequently fails on APRON due to OCaml-related install targets failing before the C targets complete. In this case, install manually:

#### Step 1 — Create the install directories

```bash
APRON_PREFIX=/path/to/apron/install   # set to your chosen prefix

mkdir -p "$APRON_PREFIX/include"
mkdir -p "$APRON_PREFIX/lib"
```

#### Step 2 — Copy headers

Copy all public headers from each subdirectory:

```bash
# Core APRON headers
cp apron/*.h "$APRON_PREFIX/include/"

# Box domain headers
cp box/*.h "$APRON_PREFIX/include/"

# Octagon domain headers
cp octagons/*.h "$APRON_PREFIX/include/"

# New Polka (convex polyhedra) headers
cp newpolka/*.h "$APRON_PREFIX/include/"

# PPL wrapper headers (if HAS_PPL = 1)
cp ppl/*.h "$APRON_PREFIX/include/"

# Products domain headers
cp products/*.h "$APRON_PREFIX/include/"

# ITV (interval) internal headers (needed by some NIKOS components)
cp itv/*.h "$APRON_PREFIX/include/"
```

#### Step 3 — Copy static libraries

Copy each `.a` file that was built:

```bash
# Core library
cp apron/libapron.a "$APRON_PREFIX/lib/"

# Box domain (double and MPQ variants)
cp box/libboxD.a    "$APRON_PREFIX/lib/"
cp box/libboxMPQ.a  "$APRON_PREFIX/lib/"

# Octagon domain (double and MPQ variants)
cp octagons/liboctD.a   "$APRON_PREFIX/lib/"
cp octagons/liboctMPQ.a "$APRON_PREFIX/lib/"

# New Polka (MPQ and long long variants)
cp newpolka/libpolkaMPQ.a  "$APRON_PREFIX/lib/"
cp newpolka/libpolkaRll.a  "$APRON_PREFIX/lib/"

# PPL wrapper libraries (if built)
cp ppl/libap_ppl.a     "$APRON_PREFIX/lib/"
cp ppl/libap_pkgrid.a  "$APRON_PREFIX/lib/"

# ITV library
cp itv/libitvMPQ.a "$APRON_PREFIX/lib/"

# Products / PkGrid libraries (if built)
cp products/libap_pkgridD.a   "$APRON_PREFIX/lib/"  2>/dev/null || true
cp products/libap_pkgridMPQ.a "$APRON_PREFIX/lib/"  2>/dev/null || true

# Compatibility aliases (some NIKOS versions look for these names)
cp box/libboxD.a     "$APRON_PREFIX/lib/libbox.a"    2>/dev/null || true
cp octagons/liboctD.a "$APRON_PREFIX/lib/liboct.a"  2>/dev/null || true
cp newpolka/libpolkaMPQ.a "$APRON_PREFIX/lib/libpolka.a" 2>/dev/null || true
```

> **Note:** The `2>/dev/null || true` at the end of optional copies suppresses errors for files that were not built (e.g., if PPL was not available). Check for these files explicitly before assuming PPL support is available.

#### Step 4 — Verify the install tree

```bash
ls "$APRON_PREFIX/include/"
# Should list: ap_abstract0.h, ap_coeff.h, box.h, oct.h, pk.h, ap_ppl.h, ...

ls "$APRON_PREFIX/lib/"
# Should list: libapron.a, libboxD.a, libboxMPQ.a, liboctD.a, liboctMPQ.a,
#              libpolkaMPQ.a, libpolkaRll.a, libap_ppl.a, libitvMPQ.a, ...
```

---

## 8. Rebuild NIKOS with APRON Support

Now that APRON is installed, rebuild NIKOS to enable APRON domain support.

### 8.1 Navigate to the NIKOS Build Directory

```bash
cd /path/to/nikos/build
```

### 8.2 Re-run CMake with APRON_ROOT

```bash
cmake \
    -DAPRON_ROOT=/path/to/apron/install \
    -DLLVM_CONFIG_EXECUTABLE=$(which llvm-config-20) \
    ..
```

| CMake Variable | Description |
|----------------|-------------|
| `APRON_ROOT` | Root of the APRON install tree (must contain `include/` and `lib/`) |
| `LLVM_CONFIG_EXECUTABLE` | Path to `llvm-config-20` (unchanged from your initial build) |

You should see CMake output like:

```
-- Found APRON: /path/to/apron/install/lib/libapron.a
-- APRON include dir: /path/to/apron/install/include
-- APRON libraries: libapron.a;libboxD.a;liboctD.a;...
-- Building with APRON support: YES
```

If the output instead says `Building with APRON support: NO`, verify that `APRON_ROOT` contains both `include/ap_abstract0.h` and `lib/libapron.a`.

### 8.3 Rebuild and Reinstall

```bash
make -j$(nproc)
make install
```

---

## 9. Testing APRON Domains

### 9.1 Prepare a Test Bitcode File

Use the same test program from the main installation guide:

```bash
echo 'int main(){ int *p = 0; return *p; }' > test.c
clang-20 -c -emit-llvm -g -O0 test.c -o test.bc
ikos-pp -domain=apron-octagon test.bc -o test.pp.bc
```

### 9.2 Run the Analyzer with an APRON Domain

```bash
ikos-analyzer \
    -a=boa \
    -d=apron-octagon \
    -entry-points=main \
    -proc=inter \
    test.pp.bc \
    -o out.db
```

| Flag | Meaning |
|------|---------|
| `-a=boa` | Enable buffer overflow / bounds-of-array checker |
| `-d=apron-octagon` | Use the APRON octagon abstract domain |
| `-entry-points=main` | Start analysis at `main()` |
| `-proc=inter` | Use inter-procedural analysis mode |
| `test.pp.bc` | Preprocessed LLVM bitcode (from `ikos-pp`) |
| `-o out.db` | Output SQLite database |

### 9.3 View the Report

```bash
ikos out.db
```

### 9.4 List Available APRON Domains

```bash
ikos-analyzer --help | grep apron
```

You should see entries like:

```
  apron-interval
  apron-octagon
  apron-polka-polyhedra
  apron-polka-linear-equalities
  apron-ppl-polyhedra       (only if HAS_PPL = 1)
  apron-ppl-linear-congruences (only if HAS_PPL = 1)
  apron-pkgrid-polyhedra-linear-congruences (only if HAS_PPL = 1)
```

---

## 10. Known Issues and Limitations

### ⚠️ APRON Is NOT Thread-Safe

APRON's internal data structures use global mutable state that is not protected by locks. Specifically:

- The GMP (GNU Multi-Precision) allocator state used by many APRON operations is not thread-safe
- The PPL library shares global initialization state

**Consequence:** When using any `apron-*` domain with NIKOS, you **must not** run the analyzer with parallel worker threads or parallel analysis jobs.

**Mitigation:** Always run `ikos-analyzer` without the `-j` (parallel jobs) flag when using APRON domains:

```bash
# CORRECT — single-threaded when using APRON
ikos-analyzer -a=boa -d=apron-octagon -entry-points=main test.pp.bc -o out.db

# INCORRECT — may crash or produce wrong results
ikos-analyzer -a=boa -d=apron-octagon -j4 -entry-points=main test.pp.bc -o out.db
```

### PPL Domains May Be Absent

If you built APRON with `HAS_PPL = 1` but `libppl.so` is not on your library path at runtime, APRON will fail to initialize PPL domains. If you see:

```
ikos-analyzer: error: failed to initialize APRON PPL domain
```

Ensure `libppl` is installed:

```bash
sudo apt-get install -y libppl-dev
# On runtime (not dev) systems:
sudo apt-get install -y libppl15   # Ubuntu 22.04
sudo apt-get install -y libppl15   # Ubuntu 24.04
```

### APRON Polyhedral Domains Are Slow

Convex polyhedra analysis (`apron-polka-polyhedra`, `apron-ppl-polyhedra`) can be **orders of magnitude slower** than interval analysis on programs with many loop variables. For large codebases, prefer `apron-octagon` as a balanced starting point.

### APRON Does Not Support All NIKOS Analysis Checkers

Not all NIKOS checkers are compatible with every APRON domain. If a checker is unsupported for a given domain, NIKOS will report a warning and fall back to the interval domain for that specific check. Consult the NIKOS documentation for checker/domain compatibility matrices.

---

## 11. Troubleshooting

### CMake cannot find APRON: `Could not find APRON`

Verify that your APRON install tree is correctly populated:

```bash
ls /path/to/apron/install/include/ | head
# Should show: ap_abstract0.h, ap_coeff.h, ...

ls /path/to/apron/install/lib/ | head
# Should show: libapron.a, libboxD.a, ...
```

If either directory is empty, repeat the [manual install steps](#72-manual-installation-fallback).

### `make` fails with errors in `mlapronidl` or `apron_caml`

These are OCaml binding targets. Set the OCaml variables to blank in `Makefile.config` and re-run make:

```makefile
HAS_OCAML =
HAS_OCAMLOPT =
HAS_OCAMLFIND =
```

Then:

```bash
make clean
make -j$(nproc)
```

### `make install` stops with an OCaml install error

Skip `make install` and use the [manual installation procedure](#72-manual-installation-fallback) instead.

### APRON build fails with `mpfr.h: No such file or directory`

Install the MPFR development package:

```bash
sudo apt-get install -y libmpfr-dev
```

### APRON build fails with `ppl.hh: No such file or directory`

Either install PPL dev or disable PPL in `Makefile.config`:

```bash
# Option A: install PPL
sudo apt-get install -y libppl-dev

# Option B: disable PPL
# In Makefile.config, set:
HAS_PPL = 0
```

### Linker error: `undefined reference to ppl_*` during NIKOS build

This means NIKOS found the APRON headers with PPL support compiled in, but cannot find the PPL library at link time. Install `libppl-dev`:

```bash
sudo apt-get install -y libppl-dev
```

Or rebuild APRON with `HAS_PPL = 0` if PPL support is not needed.

### `ikos-analyzer` segfaults when using an APRON domain

Most likely caused by a thread-safety violation. Ensure you are not using `-j` / parallel jobs with APRON domains. See [Section 10](#10-known-issues-and-limitations).

---

## 12. Available Domains Reference

The following lists all static library files built by APRON and their corresponding abstract domains:

| Library File | Domain | Description |
|---|---|---|
| `libapron.a` | (core) | Core APRON API framework — always required |
| `libboxD.a` | Box/Interval (double) | Interval domain with double-precision coefficients |
| `libboxMPQ.a` | Box/Interval (GMP) | Interval domain with arbitrary-precision GMP coefficients |
| `liboctD.a` | Octagon (double) | Octagon domain with double-precision coefficients |
| `liboctMPQ.a` | Octagon (GMP) | Octagon domain with GMP coefficients |
| `libpolkaMPQ.a` | New Polka Polyhedra (GMP) | Convex polyhedra (MPQ coefficients) |
| `libpolkaRll.a` | New Polka Polyhedra (long long) | Convex polyhedra (machine-integer coefficients, faster) |
| `libap_ppl.a` | PPL Polyhedra / Grid | Wrapper around the Parma Polyhedra Library |
| `libap_pkgrid.a` | PkGrid | Product of polyhedra and linear congruences |
| `libitvMPQ.a` | ITV (internal) | Internal interval representation used by other domains |
| `libap_pkgridD.a` | PkGrid (double) | Double-precision PkGrid variant |
| `libap_pkgridMPQ.a` | PkGrid (GMP) | GMP-precision PkGrid variant |
| `libbox.a` | Box (alias) | Alias for `libboxD.a` (compatibility) |
| `liboct.a` | Octagon (alias) | Alias for `liboctD.a` (compatibility) |
| `libpolka.a` | Polka (alias) | Alias for `libpolkaMPQ.a` (compatibility) |

---

## See Also

- [NIKOS v1.0.0 — Ubuntu 22.04 Installation Guide](./UBUNTU_22.04.md)
- [NIKOS v1.0.0 — Ubuntu 24.04 Installation Guide](./UBUNTU_24.04.md)
- [APRON project home](https://antoinemine.github.io/Apron/doc/)
- [APRON GitHub repository](https://github.com/antoinemine/apron)
