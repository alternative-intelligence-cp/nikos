Name:           nikos
Version:        1.0.0
Release:        1%{?dist}
Summary:        Static Analyzer for C/C++ based on Abstract Interpretation (LLVM 20)

License:        NOSA 1.3
URL:            https://github.com/alternative-intelligence-cp/nikos
Source0:        https://github.com/alternative-intelligence-cp/nikos/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.20
BuildRequires:  gcc-c++
BuildRequires:  llvm-devel >= 20
BuildRequires:  clang-devel >= 20
BuildRequires:  boost-devel >= 1.71
BuildRequires:  gmp-devel
BuildRequires:  sqlite-devel
BuildRequires:  tbb-devel
BuildRequires:  libedit-devel
BuildRequires:  zlib-devel
BuildRequires:  python3

Requires:       boost-filesystem
Requires:       boost-regex
Requires:       boost-thread
Requires:       gmp
Requires:       sqlite-libs
Requires:       tbb
Requires:       libedit
Requires:       zlib
Requires:       python3

%description
NIKOS (New Inference Kernel for Open Static Analyzers) is a production-ready
static analyzer for C/C++ based on Abstract Interpretation. It is a modernized
fork of NASA's IKOS, ported from LLVM 14 to LLVM 20.

NIKOS detects:
  - Buffer overflows (boa)
  - Null pointer dereferences (nullity)
  - Division by zero (dbz)
  - Use of uninitialized variables (uva)
  - Signed/unsigned integer overflows (sio, uio)
  - Double-free / use-after-free (dfa)
  - And 10 more defect categories

All 64 regression tests pass. Optional APRON support adds 13 relational
abstract domains for higher-precision analysis.

%prep
%autosetup

%build
%cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_CONFIG_EXECUTABLE=%{_bindir}/llvm-config
%cmake_build

%install
%cmake_install

%files
%license LICENSE.txt
%doc README.md CHANGELOG.md TROUBLESHOOTING.md
%{_bindir}/ikos
%{_bindir}/ikos-analyzer
%{_bindir}/ikos-pp
%{_bindir}/ikos-import
%{_bindir}/ikos-report
%{_bindir}/ikos-view
%{_bindir}/ikos-scan
%{_bindir}/ikos-scan-cc
%{_bindir}/ikos-scan-c++
%{_bindir}/ikos-scan-extract
%{_bindir}/ikos-config
%{_libdir}/ikos/
%{_datadir}/ikos/

%changelog
* Sat Jun 14 2026 Alternative Intelligence <contact@alternativeintelligence.co> - 1.0.0-1
- Initial production release v1.0.0
- LLVM 20 support (opaque pointers, new PassBuilder, LLVM 20 debug info)
- 64/64 regression tests passing
- Optional APRON abstract domain library support (13 relational domains)
