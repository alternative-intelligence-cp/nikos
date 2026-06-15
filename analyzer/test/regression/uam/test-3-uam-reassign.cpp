// test-3-uam-reassign.cpp
//
// Use-after-move test: reassignment makes the pointer valid again.
// After move, a new value is assigned to the moved-from pointer.
//
// Expected: safe
#include <memory>

int main() {
    auto p = std::make_unique<int>(1);
    auto q = std::move(p);  // p is moved-from
    p = std::make_unique<int>(2); // p is now valid again
    return *p + *q;              // safe: both are valid
}
