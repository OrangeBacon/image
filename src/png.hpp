#pragma once
#include <numeric>
#include <cmath>
#include <cstdint>

struct Pixel {
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t a;

    Pixel(uint16_t r, uint16_t g, uint16_t b, uint16_t a = 0) :
        r(r), g(g), b(b), a(a) {};

    // hsv conversion
    Pixel(double H, double S, double V);
};
