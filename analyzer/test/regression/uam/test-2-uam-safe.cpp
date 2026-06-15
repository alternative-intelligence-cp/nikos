// test-2-uam-safe.cpp
//
// Use-after-move test: safe case.
// A unique_ptr is moved, and only the new owner (q) is accessed.
//
// Expected: safe
#include <memory>

int main() {
    auto p = std::make_unique<int>(42);
    auto q = std::move(p);  // q now owns the int
    return *q;              // safe: access through the new owner
}
