#include <stdexcept>
#include <iostream>

void may_throw(int x) {
    if (x < 0) {
        throw std::runtime_error("Negative value not allowed");
    }
}

int main(int argc, char** argv) {
    int result = 0;
    try {
        may_throw(argc - 2);
        result = 1;
    } catch (const std::exception& e) {
        result = -1;
    }
    return result;
}
