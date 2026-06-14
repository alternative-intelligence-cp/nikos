# NIKOS v1.0.0 — Installation Guide for Ubuntu 24.04 LTS (Noble Numbat)

> **Platform:** Ubuntu 24.04 LTS (Noble Numbat)  
> **Toolchain:** LLVM 20 / Clang 20  
> **Last updated:** 2026-06-14

---

## Table of Contents

1. [Overview](#1-overview)
2. [System Requirements](#2-system-requirements)
3. [Install LLVM 20 and Clang 20](#3-install-llvm-20-and-clang-20)
4. [Install Other Dependencies](#4-install-other-dependencies)
5. [Clone and Build NIKOS](#5-clone-and-build-nikos)
6. [Configure Your PATH](#6-configure-your-path)
7. [Smoke Test](#7-smoke-test)
8. [Optional: APRON Numerical Domains](#8-optional-apron-numerical-domains)
9. [Troubleshooting](#9-troubleshooting)

---

## 1. Overview

NIKOS is a static analyzer for C/C++ programs built on top of LLVM. It uses abstract interpretation to detect bugs such as buffer overflows, null-pointer dereferences, integer overflows, and more. This guide covers a complete installation from scratch on Ubuntu 24.04 LTS.

> **⚠️ Important — Ubuntu 24.04 ships LLVM 17 by default.**
>
> Ubuntu 24.04 Noble Numbat includes `llvm-17` and `clang-17` in its default `apt` repositories. NIKOS v1.0.0 requires **LLVM 20**, which is **not available** from the standard Ubuntu 24.04 repositories. You **must** add the official LLVM apt repository and explicitly install version 20.
>
> The procedure is identical to Ubuntu 22.04 — the LLVM installation script handles repository detection automatically. The rest of this guide mirrors the [Ubuntu 22.04 guide](./UBUNTU_22.04.md) with any 24.04-specific notes called out explicitly.

---

## 2. System Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| CPU cores | 2 | 4+ |
| RAM | 4 GB | 8 GB+ |
| Disk space | 5 GB | 10 GB+ |
| OS | Ubuntu 24.04 LTS (64-bit) | Ubuntu 24.04 LTS (64-bit) |
| LLVM | 20 | 20 |
| CMake | 3.14+ | 3.28+ (default on 24.04) |
| GCC/G++ | 10+ | 13 (default on 24.04) |

> **Note on GCC version:** Ubuntu 24.04 ships GCC 13 by default, which is fully compatible with NIKOS v1.0.0's build requirements.

---

## 3. Install LLVM 20 and Clang 20

### 3.1 Why the Standard Repositories Are Not Sufficient

Running `sudo apt-get install llvm clang` on Ubuntu 24.04 installs **LLVM 17/Clang 17**, which will cause the NIKOS CMake configuration step to fail:

```
-- Could not find LLVM 20
-- Found LLVM 17.x.x, but NIKOS requires LLVM 20
CMake Error: ...
```

You must use the official LLVM apt repository.

### 3.2 Download and Run the LLVM Installation Script

```bash
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 20
```

The script automatically detects that you are running Ubuntu 24.04 (Noble), adds the correct LLVM apt repository (`deb http://apt.llvm.org/noble/ llvm-toolchain-noble-20 main`), imports the LLVM GPG key, and installs `llvm-20`, `clang-20`, `lld-20`, and related tools.

> **Note:** The script requires `wget`, `gnupg`, and `software-properties-common`. If any are missing:
> ```bash
> sudo apt-get update
> sudo apt-get install -y wget gnupg software-properties-common
> ```

### 3.3 Verify the Installation

```bash
llvm-config-20 --version
# Expected output: 20.x.x

clang-20 --version
# Expected output: clang version 20.x.x (...)
```

> **Caution:** Do not confuse `/usr/bin/llvm-config` (which points to LLVM 17 on 24.04) with `/usr/bin/llvm-config-20`. Always use the version-suffixed binary, or update the alternatives as shown below.

### 3.4 (Optional) Set LLVM 20 as Default via update-alternatives

If you want `clang`, `clang++`, and `llvm-config` to point to version 20 system-wide:

```bash
sudo update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-20 100
sudo update-alternatives --install /usr/bin/clang       clang       /usr/bin/clang-20       100
sudo update-alternatives --install /usr/bin/clang++     clang++     /usr/bin/clang++-20     100
```

To switch between versions interactively:

```bash
sudo update-alternatives --config clang
```

---

## 4. Install Other Dependencies

Install the remaining build and runtime dependencies via apt:

```bash
sudo apt-get update
sudo apt-get install -y \
    cmake \
    libboost-all-dev \
    libgmp-dev \
    libsqlite3-dev \
    libtbb-dev \
    libedit-dev \
    zlib1g-dev \
    python3
```

> **Ubuntu 24.04 notes:**
> - `cmake` 3.28 is available in the default 24.04 repos — no PPA needed.
> - `libtbb-dev` on 24.04 provides oneTBB (version 2021.x), which is compatible with NIKOS v1.0.0.
> - All other packages are available from the standard 24.04 repos at compatible versions.

### Dependency Reference

| Package | Purpose |
|---------|---------|
| `cmake` | Build system generator |
| `libboost-all-dev` | Boost C++ libraries (filesystem, thread, regex, etc.) |
| `libgmp-dev` | GNU Multiple Precision Arithmetic (required for abstract domains) |
| `libsqlite3-dev` | SQLite3 development headers (analysis result database) |
| `libtbb-dev` | Intel/oneAPI Threading Building Blocks (parallel analysis passes) |
| `libedit-dev` | Command-line editing support for the NIKOS REPL |
| `zlib1g-dev` | Zlib compression (LLVM bitcode I/O) |
| `python3` | Python 3 runtime (analysis scripts and utilities) |

---

## 5. Clone and Build NIKOS

The build procedure is identical to Ubuntu 22.04. Steps are reproduced in full for convenience.

### 5.1 Clone the Repository

```bash
git clone https://github.com/your-org/nikos.git
cd nikos
```

> Replace `https://github.com/your-org/nikos.git` with the actual repository URL if different.

### 5.2 Create the Build Directory

Always build out-of-source:

```bash
mkdir build
cd build
```

### 5.3 Configure with CMake

```bash
cmake -DLLVM_CONFIG_EXECUTABLE=$(which llvm-config-20) ..
```

> **Ubuntu 24.04 reminder:** `which llvm-config-20` should return `/usr/bin/llvm-config-20`. If it returns `/usr/bin/llvm-config` (pointing to LLVM 17), use the full versioned path explicitly:
> ```bash
> cmake -DLLVM_CONFIG_EXECUTABLE=/usr/bin/llvm-config-20 ..
> ```

**Key CMake Variables**

| Variable | Description | Example |
|----------|-------------|---------|
| `LLVM_CONFIG_EXECUTABLE` | Path to `llvm-config-20` binary | `/usr/bin/llvm-config-20` |
| `CMAKE_INSTALL_PREFIX` | Install destination (default: `../install`) | `/opt/nikos` |
| `CMAKE_BUILD_TYPE` | Build type: `Release` or `Debug` | `Release` |

To customize the install prefix and build type:

```bash
cmake \
    -DLLVM_CONFIG_EXECUTABLE=$(which llvm-config-20) \
    -DCMAKE_INSTALL_PREFIX=/opt/nikos \
    -DCMAKE_BUILD_TYPE=Release \
    ..
```

### 5.4 Compile

```bash
make -j$(nproc)
```

`$(nproc)` uses all available CPU cores. Compilation typically takes 5–15 minutes depending on hardware.

> **Tip:** If the build fails with out-of-memory errors, reduce parallel jobs:
> ```bash
> make -j2
> ```

### 5.5 Install

```bash
make install
```

By default, files are installed into `../install` relative to the build directory (i.e., `nikos/install/`). If you specified a custom `CMAKE_INSTALL_PREFIX`, files go there instead.

After installation, the layout should look like:

```
<install_prefix>/
├── bin/
│   ├── ikos
│   ├── ikos-analyzer
│   ├── ikos-pp
│   └── ...
├── include/
│   └── ikos/
└── lib/
    └── ...
```

---

## 6. Configure Your PATH

Add the NIKOS `bin/` directory to your PATH so the tools are available from any shell.

### If you used the default install prefix (`nikos/install/`)

Determine the absolute path:

```bash
# From inside the build directory:
realpath ../install/bin
```

Then add to your shell profile (`~/.bashrc` for Bash, `~/.zshrc` for Zsh):

```bash
echo 'export PATH=/path/to/nikos/install/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

For example, if you cloned NIKOS into `/home/user/nikos`:

```bash
echo 'export PATH=/home/user/nikos/install/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

### If you used a custom prefix (e.g., `/opt/nikos`)

```bash
echo 'export PATH=/opt/nikos/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

### Verify

```bash
which ikos
ikos --version
```

---

## 7. Smoke Test

This section verifies that NIKOS is installed and working correctly by analyzing a small C program with a deliberate null-pointer dereference.

### 7.1 Write the Test Program

```bash
echo 'int main(){ int *p = 0; return *p; }' > test.c
```

### 7.2 Compile to LLVM Bitcode

```bash
# Compile to bitcode:
clang-20 -c -emit-llvm -g -O0 test.c -o test.bc

# (Optional) Run the NIKOS preprocessor:
ikos-pp -domain=interval test.bc -o test.pp.bc
```

### 7.3 Run the Analyzer

```bash
ikos-analyzer \
    -a=nullity \
    -d=interval \
    -entry-points=main \
    test.bc \
    -o out.db
```

| Flag | Meaning |
|------|---------|
| `-a=nullity` | Enable the null-pointer dereference checker |
| `-d=interval` | Use the interval abstract domain |
| `-entry-points=main` | Start analysis at `main()` |
| `test.bc` | Input LLVM bitcode file |
| `-o out.db` | Write results to SQLite database |

### 7.4 View the Report

```bash
ikos out.db
```

You should see output similar to:

```
# Results
test.c: In function 'main':
test.c:1:30: error: null pointer dereference
  int main(){ int *p = 0; return *p; }
                                  ^
```

If you see this error reported, NIKOS is working correctly.

---

## 8. Optional: APRON Numerical Domains

APRON is a C library providing 13 additional abstract domains. It is optional but recommended for more precise analysis.

### 8.1 Install APRON Prerequisites

```bash
sudo apt-get install -y libmpfr-dev libppl-dev
```

### 8.2 Build APRON from Source

For the complete APRON build procedure, see the dedicated guide:

📄 **[APRON Build Guide](./APRON.md)**

The APRON guide covers:
- Cloning and configuring the APRON source tree
- `Makefile.config` settings (including OCaml-free build)
- Manual installation workarounds for broken `make install`
- Rebuilding NIKOS with APRON support
- Known issues (thread safety, domain availability)

### 8.3 Rebuild NIKOS with APRON Support

After building and installing APRON, re-run CMake from your NIKOS build directory:

```bash
cd /path/to/nikos/build

cmake \
    -DAPRON_ROOT=/path/to/apron/install \
    -DLLVM_CONFIG_EXECUTABLE=$(which llvm-config-20) \
    ..

make -j$(nproc)
make install
```

---

## 9. Troubleshooting

### CMake finds LLVM 17 instead of LLVM 20

This is the most common Ubuntu 24.04-specific issue. Ubuntu 24.04 installs LLVM 17 by default, and CMake may pick it up if `llvm-config` is not version-pinned.

**Fix:** Always pass the version-suffixed binary explicitly:

```bash
cmake -DLLVM_CONFIG_EXECUTABLE=/usr/bin/llvm-config-20 ..
```

Alternatively, configure `update-alternatives` as described in [Section 3.4](#34-optional-set-llvm-20-as-default-via-update-alternatives).

### `llvm-config-20: command not found`

LLVM 20 is not installed. Re-run the installation script:

```bash
wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && sudo ./llvm.sh 20
```

### `make` fails with `undefined reference to ...`

Ensure all apt dependencies were installed:

```bash
sudo apt-get install -y libboost-all-dev libgmp-dev libsqlite3-dev libtbb-dev libedit-dev zlib1g-dev
```

On Ubuntu 24.04, `libtbb-dev` installs oneTBB. If you encounter oneTBB-specific API incompatibilities, check the NIKOS release notes for the minimum required oneTBB version.

### `ikos-analyzer` crashes or produces no output

Ensure the bitcode was compiled with `-g` (debug info) and `-O0` (no optimization). The analyzer requires unoptimized, annotated bitcode:

```bash
clang-20 -c -emit-llvm -g -O0 test.c -o test.bc
```

### Out-of-memory during build

Reduce the number of parallel jobs:

```bash
make -j2
# or
make -j1
```

### `make install` reports permission denied

Use `sudo make install` if installing to a system path, or specify a user-writable prefix:

```bash
cmake -DCMAKE_INSTALL_PREFIX=$HOME/nikos-install ..
make -j$(nproc)
make install
```

### Conflicts between LLVM 17 and LLVM 20 headers

If you see header conflicts during the NIKOS build, ensure that CMake is exclusively picking up LLVM 20 headers. Inspect the CMake output for the `LLVM_INCLUDE_DIRS` variable:

```
-- LLVM include dirs: /usr/lib/llvm-20/include
```

If it shows `/usr/lib/llvm-17/include` or both, re-run CMake with the explicit path as above.

---

## See Also

- [NIKOS v1.0.0 — Ubuntu 22.04 Installation Guide](./UBUNTU_22.04.md)
- [NIKOS v1.0.0 — APRON Numerical Domains Build Guide](./APRON.md)
