#include "png.hpp"

#include <numeric>
#include <cmath>
#include <iostream>
#include <type_traits>

uint16_t clamp(double val) {
    return static_cast<uint16_t>(std::round(val * UINT16_MAX));
}

static uint32_t crc32(uint32_t data, uint32_t prev = 0) {
    uint32_t polynomial = 0xEDB88320L;
    static uint32_t table[256];
    static bool tableInitialised = false;

    if (!tableInitialised) {
        tableInitialised = true;
        for (uint32_t i = 0; i <= 0xFF; i++) {
            uint32_t crc = i;
            for (uint32_t j = 0; j < 8; j++) {
                crc = (crc >> 1) ^ (-int(crc & 1) & polynomial);
            }
            table[i] = crc;
        }
    }

    return (prev >> 8) ^ table[(prev ^ data) & 0xff];
}

CrcStream CrcStream::operator<<(uint8_t data) {
    stream << data;
    crc = crc32(data, crc);
    return *this;
}

// http://www.splinter.com.au/converting-hsv-to-rgb-colour-using-c/
Pixel::Pixel(double H, double S, double V) : a(0) {
    while (H < 0) { H += 360; }
    while (H >= 360) { H -= 360; }

    double R;
    double G;
    double B;

    if (V <= 0) {
        R = G = B = 0;
    } else if (S <= 0) {
        R = G = B = V;
    } else {
        double hf = H / 60.0;
        int i = static_cast<int>(std::floor(hf));
        double f = hf - i;
        double pv = V * (1 - S);
        double qv = V * (1 - S * f);
        double tv = V * (1 - S * (1 - f));

        switch (i) {
            // mainly red
        case 6:
        case 0:
            R = V;
            G = tv;
            B = pv;
            break;

            // mainly green
        case 1:
            R = qv;
            G = V;
            B = pv;
            break;
        case 2:
            R = pv;
            G = V;
            B = tv;
            break;

            // mainly blue
        case 3:
            R = pv;
            G = qv;
            B = V;
            break;
        case 4:
            R = tv;
            G = pv;
            B = V;
            break;

            // mainly red (again)
        case 5:
        case -1:
            R = V;
            G = pv;
            B = qv;
            break;

            // if error
        default:
            R = G = B = V; // greyscale
        }
    }

    r = clamp(R);
    g = clamp(G);
    b = clamp(B);
}

PNGImage::PNGImage(std::vector<std::vector<Pixel>>& data) : chunks(), hasError(false) {
    size_t width = data.size();
    if (width < 1) {
        hasError = true;
        return;
    }

    size_t height = data[0].size();
    if (height < 1) {
        hasError = true;
        return;
    }

    for (auto& row : data) {
        if (row.size() != height) {
            hasError = true;
            return;
        }
    }

    chunks.push_back(std::make_unique<Chunks::IHDR>(
        static_cast<uint32_t>(width), 
        static_cast<uint32_t>(height)));
    chunks.push_back(std::make_unique<Chunks::IEND>());
}

void PNGImage::write(std::ostream& file) {
    if (hasError) return;
    std::cout << "chunk count: " << chunks.size() << "\n";
    file << "\211PNG\r\n\032\n";

    for (auto& chunk : chunks) {
        chunk->write(file);
    }
}

template<typename t>
static void WriteBigEndian(t& file, uint32_t num) {
    file << (uint8_t)((num >> 24) & 0xff)
        << (uint8_t)((num >> 16) & 0xff)
        << (uint8_t)((num >> 8) & 0xff)
        << (uint8_t)(num & 0xff);
}

Chunk::Chunk(uint32_t length, std::string type, uint32_t crc) :
    length(length), type(type), crc(crc) {};

void Chunk::write(std::ostream& file) {
    WriteBigEndian(file, length);
    file << type;

    CrcStream stream(file);
    compute(stream);

    WriteBigEndian(file, stream.get_crc());
}

void Chunks::IHDR::compute(CrcStream& out) {
    WriteBigEndian(out, width);
    WriteBigEndian(out, height);
    out << bit_depth
        << color_type
        << compression_method
        << filter_method
        << interlace_method;
}
