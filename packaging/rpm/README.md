# Building NIKOS RPM Package

The `nikos.spec` file in this directory builds a NIKOS RPM package for Fedora
and RHEL/AlmaLinux/Rocky Linux.

## Prerequisites

Building an RPM requires a Fedora or RHEL-based system with:

```bash
sudo dnf install -y rpm-build rpmdevtools clang llvm-devel cmake \
    boost-devel gmp-devel sqlite-devel tbb-devel python3
```

You also need LLVM 20. On Fedora 40+:

```bash
sudo dnf install -y llvm20-devel clang20-devel
```

## Building

```bash
# Set up RPM build tree
rpmdev-setuptree

# Download the source tarball
spectool -g -R nikos.spec

# Build the RPM
rpmbuild -ba nikos.spec
```

The output `.rpm` will be in `~/rpmbuild/RPMS/x86_64/`.

## Installing

```bash
sudo rpm -ivh ~/rpmbuild/RPMS/x86_64/nikos-1.0.0-1.x86_64.rpm

# Verify
ikos-analyzer --help | head -3
```

## Note

On Ubuntu (the primary development platform for NIKOS), the RPM spec file
can be linted with `rpmlint` but not actually built. To build the RPM, use
a Fedora container:

```bash
docker run --rm -v $(pwd):/src:ro fedora:40 bash -c "
  dnf install -y rpm-build rpmdevtools llvm-devel cmake boost-devel \
    gmp-devel sqlite-devel tbb-devel python3 clang &&
  rpmdev-setuptree &&
  cp /src/nikos.spec ~/rpmbuild/SPECS/ &&
  spectool -g -R ~/rpmbuild/SPECS/nikos.spec &&
  rpmbuild -ba ~/rpmbuild/SPECS/nikos.spec
"
```
