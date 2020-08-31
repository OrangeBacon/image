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
            row.push_back({ 360.0 / width * x, 0.5, 1.0 / height * y });
        }
        data.push_back(row);
    }
    
    std::cout << "width: " << data.size() << "\n"
        << "height: " << data[0].size() << "\n";

    PNGImage image(data);
    image.creation_time("30/08/2020");
    image.description(u8"my image\U0001F496", "en-gb", "Describer!!");
    image.write(std::ofstream("image.png", std::ios::binary));

    return 0;
}
