#pragma once
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <string>
#define CHECK(expr) do { if (!(expr)) throw std::runtime_error( \
    std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #expr); } while (0)

template<class F> int RunTests(const char *name, F run)
{
    try { run(); std::printf("PASS: %s\n", name); return 0; }
    catch (const std::exception& e) { std::fprintf(stderr, "FAIL: %s\n", e.what()); return 1; }
}
