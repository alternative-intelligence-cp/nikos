// test-2-lambda-dangling-ref.cpp
//
// Lambda test: capture by reference — lambda escapes the scope of the
// captured variable. Calling the returned lambda is a use-after-return.
//
// Note: With LLVM 20 opaque pointers the nullity/UAF checker may report
// this as safe due to precision loss in pointer tracking. The test
// expects 'error' but accepts 'safe' as PASS_IMPROVE under opaque-ptr
// tolerance mode.
//
// Expected: error (use-after-return / dangling reference)
#include <functional>

std::function<int()> make_lambda() {
    int x = 42;
    return [&x]() { return x; }; // capture by reference: dangling after return
}

int main() {
    auto f = make_lambda();
    return f(); // use-after-return: x is on make_lambda's stack frame
}
