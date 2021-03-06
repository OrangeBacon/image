#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <array>

#include "png.hpp"

int main() {
    std::vector<std::vector<Pixel>> data{};

    size_t width = 1920;
    size_t height = 1080;
    for (size_t x = 0; x < width; x++) {
        auto row = std::vector<Pixel>();
        for (size_t y = 0; y < height; y++) {
            row.push_back(Pixel::HSV(360.0 / width * x, 0.5, 1.0 / height * y));
        }
        data.push_back(row);
    }
    
    std::cout << "width: " << data.size() << "\n"
        << "height: " << data[0].size() << "\n";

    PNGImage image{};
    image.background({ 0, 0, 0 });
    image.description("my image\U0001F496", "en-gb", "Describer!!");
    image.creation_time();
    image.modification_time();
    image.no_alpha();
    image.data(std::move(data));
    image.write(std::ofstream("image.png", std::ios::binary));

    return 0;
}
