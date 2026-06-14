# Contributing to NIKOS

Thank you for your interest in contributing to NIKOS! We welcome contributions from
the community — whether it's bug reports, feature requests, documentation improvements,
or code changes.

## Getting Started

### Prerequisites

Ensure the following dependencies are installed:

| Dependency | Version       |
|------------|---------------|
| LLVM/Clang | 20            |
| CMake      | ≥ 3.16        |
| Boost      | ≥ 1.71        |
| GMP        | latest stable |
| SQLite3    | latest stable |
| TBB        | latest stable |
| Python     | ≥ 3.8         |

### Building from Source

```bash
git clone https://github.com/<your-fork>/nikos.git
cd nikos
mkdir build && cd build
cmake -DLLVM_CONFIG_EXECUTABLE=$(which llvm-config-20) ..
make -j$(nproc)
```

### Running Tests

```bash
ctest --test-dir build --output-on-failure
```

All 64 tests must pass before submitting a pull request.

## How to Contribute

1. **Fork** the repository on GitHub.
2. **Create a branch** from `main` (see [Branch Naming](#branch-naming) below).
3. **Make your changes** — keep commits focused and well-described.
4. **Run `ctest`** and verify all 64 tests pass.
5. **Submit a pull request** against `main` with a clear description of your changes.

## Branch Naming

| Type        | Pattern                | Example            |
|-------------|------------------------|--------------------|
| Development | `dev-X.Y.x`           | `dev-0.5.x`        |
| Feature     | `feature/description`  | `feature/add-taint` |
| Bug fix     | `fix/description`      | `fix/null-deref`    |

## Code Style

- **Standard:** C++17.
- **Formatting:** Use the project's `.clang-format` configuration. Run `clang-format -i`
  on all changed files before committing.
- **Static analysis:** Run `.clang-tidy` checks on modified code.
- **Naming:** Use descriptive variable and function names that follow existing conventions.
- **Documentation:** Document all public APIs with Doxygen-style comments.

```cpp
/// \brief Analyze the control-flow graph for null pointer dereferences.
/// \param cfg The control-flow graph to analyze.
/// \return A list of detected null-dereference warnings.
std::vector<Warning> analyzeNullDeref(const CFG& cfg);
```

## Testing

- **Add tests** for every new feature or bug fix.
- **Run `ctest`** locally before pushing: `ctest --test-dir build --output-on-failure`
- **Ensure no regressions** — all 64 existing tests must continue to pass.

## Reporting Issues

Please use [GitHub Issues](../../issues) to report bugs or request features. Include:

- **OS and version** (e.g., Ubuntu 24.04)
- **LLVM version** (e.g., LLVM 20.1)
- **Steps to reproduce** the issue
- **Expected behavior** vs. **actual behavior**
- **Relevant log output** or error messages (use code blocks)

## License

All contributions to NIKOS are subject to the
[NASA Open Source Agreement (NOSA) v1.3](LICENSE.txt). By submitting a pull request,
you agree that your contributions will be licensed under the same terms.

---

Thank you for helping improve NIKOS! If you have questions, feel free to open an issue
or start a discussion.
