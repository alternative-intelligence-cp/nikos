// test-4-uam-write.cpp
//
// Use-after-move test: write through moved-from pointer.
// This is a write operation through the moved-from ptr, which is
// also a use-after-move (stores to the memory p points at after p is emptied).
//
// Expected: error (use after move — write through moved-from pointer)
#include <memory>

int main() {
    auto p = std::make_unique<int>(1);
    auto q = std::move(p);
    *p = 99;  // write-after-move: p.get() is null after move
    return *q;
}
