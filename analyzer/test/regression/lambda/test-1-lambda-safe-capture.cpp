// test-1-lambda-safe-capture.cpp
//
// Lambda test: capture by value — no dangling reference possible.
// The captured int is copied into the closure object; the original
// local variable going out of scope does not affect the lambda.
//
// Expected: safe
#include <functional>

std::function<int()> make_lambda() {
    int x = 42;
    return [x]() { return x; }; // capture by value: safe
}

int main() {
    auto f = make_lambda();
    return f() - 42; // returns 0
}
