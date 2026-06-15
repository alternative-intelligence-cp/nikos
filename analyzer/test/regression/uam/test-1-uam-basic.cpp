// test-1-uam-basic.cpp
//
// Use-after-move test: basic case.
// A unique_ptr is moved; the moved-from ptr is then dereferenced.
//
// Expected: error (use after move)
#include <utility>

struct MoveOnly {
    int* ptr;
    MoveOnly() : ptr(nullptr) {}
    MoveOnly(int* p) : ptr(p) {}
    // Move constructor
    MoveOnly(MoveOnly&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    // Move assignment
    MoveOnly& operator=(MoveOnly&& other) noexcept {
        if (this != &other) {
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
    int& operator*() const { return *ptr; }
};

int main() {
    int x = 42;
    MoveOnly p(&x);
    MoveOnly q(std::move(p));
    return *p;
}
