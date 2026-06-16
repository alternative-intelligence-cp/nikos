# Examples

Minimal C programs demonstrating NIKOS analysis capabilities.

| File | What it demonstrates |
|---|---|
| `data_race.c` | Data race detection — two threads accessing shared data with mismatched locking |
| `deadlock.c` | Deadlock detection — two threads acquiring locks in opposite order |

## Running

```bash
# Build and analyze a single example
clang-20 -c -emit-llvm -g -O0 -pthread examples/data_race.c -o data_race.bc
ikos-pp -opt=none data_race.bc -o data_race.pp.bc
ikos-analyzer -a=concurrency -d=interval -entry-points=main \
  data_race.pp.bc -o data_race.db

# Or use the ikos wrapper
ikos --analyses=concurrency examples/data_race.c
```
