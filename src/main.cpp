#include <iostream>
#include <vector>
#include <array>

#include "png.hpp"

int main() {
    std::vector<std::vector<Pixel>> image{};

    size_t width = 1920;
    size_t height = 1080;
    for (size_t x = 0; x < width; x++) {
        auto row = std::vector<Pixel>();
        for (size_t y = 0; y < height; y++) {
            row.push_back({ 360.0 / width * x, 0.5, 1.0 / height * y });
        }
        image.push_back(row);
    }
    
    std::cout << "width: " << image.size() << "\n"
        << "height: " << image[0].size() << "\n";

    return 0;
}
