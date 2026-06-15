// test-3-lambda-capture-move.cpp
//
// Lambda test: capture unique_ptr by move into the closure.
// The ownership is transferred into the closure; the original 'p' is
// moved-from (empty) but is never accessed after the capture, so there
// is no use-after-move bug.
//
// Expected: safe
#include <functional>
#include <memory>

std::function<int()> make_lambda() {
    auto p = std::make_unique<int>(99);
    // Capture p by move: p is moved into the lambda's closure object.
    // After this, p is moved-from (p.get() == nullptr), but p is not
    // used again.
    return [q = std::move(p)]() { return *q; };
}

int main() {
    auto f = make_lambda();
    return f() - 99; // returns 0
}
