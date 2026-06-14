# NIKOS Docker Guide

NIKOS provides an official Docker image for easy deployment without a local
LLVM 20 toolchain installation.

## Quick Start

```bash
# Build the image
docker build -t nikos:1.0.0 .

# Analyze a C file (mounts current directory into /work)
docker run --rm -v $(pwd):/work nikos:1.0.0 \
    -a=boa,nullity,dbz -d=interval \
    -entry-points=main -proc=inter \
    /work/my_program.pp.bc -o /work/results.db
```

## Workflow

The Docker image contains `ikos-analyzer` (the core analysis binary), `ikos-pp`
(bitcode preprocessor), and `clang-20`. The typical workflow:

```bash
# 1. Compile your file to LLVM bitcode (can do this outside Docker)
clang-20 -c -emit-llvm -g -O0 my_program.c -o my_program.bc

# 2. Preprocess with ikos-pp
docker run --rm -v $(pwd):/work --entrypoint ikos-pp nikos:1.0.0 \
    -opt=basic -entry-points=main \
    /work/my_program.bc -o /work/my_program.pp.bc

# 3. Analyze
docker run --rm -v $(pwd):/work nikos:1.0.0 \
    -a=boa,nullity,dbz -d=interval \
    -entry-points=main -proc=inter \
    /work/my_program.pp.bc -o /work/results.db

# 4. Read results (SQLite)
sqlite3 results.db "SELECT status, COUNT(*) FROM checks GROUP BY status"
```

## Docker Compose

Use the provided `docker/docker-compose.yml` for convenience:

```bash
# From the repo root
docker compose -f docker/docker-compose.yml run nikos-analyzer \
    -a=boa -d=interval -entry-points=main -proc=inter \
    /work/my_program.pp.bc -o /work/results.db
```

## Image Details

| Property | Value |
|---|---|
| Base image | `ubuntu:24.04` (runtime) |
| NIKOS install | `/opt/nikos` |
| PATH | `/opt/nikos/bin` prepended |
| Default entrypoint | `ikos-analyzer` |
| APRON | Not included (requires source build) |

## Building with APRON

To build a Docker image with APRON support, extend the Dockerfile:

```dockerfile
FROM nikos:1.0.0 AS nikos-apron
# ... APRON build steps (see doc/install/1.0/APRON.md)
```

Note: APRON adds significant build time and image size. It is not included in
the default image.

## CI Integration

```yaml
# GitHub Actions example
jobs:
  analyze:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Build NIKOS image
        run: docker build -t nikos:1.0.0 .
      - name: Compile to bitcode
        run: |
          sudo apt-get install -y clang-20
          clang-20 -c -emit-llvm -g -O0 src/main.c -o main.bc
          docker run --rm -v $(pwd):/work --entrypoint ikos-pp nikos:1.0.0 \
            -opt=basic -entry-points=main /work/main.bc -o /work/main.pp.bc
      - name: Analyze
        run: |
          docker run --rm -v $(pwd):/work nikos:1.0.0 \
            -a=boa,nullity,dbz -d=interval \
            -entry-points=main -proc=inter \
            /work/main.pp.bc -o /work/results.db
          sqlite3 results.db \
            "SELECT CASE status WHEN 2 THEN 'ERROR' WHEN 3 THEN 'WARNING' END, COUNT(*) FROM checks WHERE status > 1 GROUP BY status"
```
