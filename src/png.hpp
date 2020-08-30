#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <fstream>

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

class CrcStream {
    uint32_t crc;
    std::ostream& stream;

public:
    CrcStream(std::ostream& s) : crc(0), stream(s) {}

    CrcStream operator<<(uint8_t data);

    uint32_t get_crc() {
        return ~crc;
    }
};

// assumptions
// - running on little endian machine
// - truecolor pixels
// - 16 bit samples
// - 16 bit alpha chanel
// - not interlaced
// - uses sRGB chunk (standard RGB)
//   - implies gAMA + cHRM
// - no iCCP chunk (advanced color management)
// - no sPLT chunk (suggested pallet)
// - no PLTE chunk (pallet)

struct Chunk {
    uint32_t length; // max value = 2^31 - 1
    std::string type;
    uint32_t crc;

    Chunk(uint32_t length, std::string type, uint32_t crc = 0);

    void write(std::ostream& file);
    virtual void write_data(CrcStream& out) {};
};

struct PNGImage {
    bool hasError;

    std::vector<std::unique_ptr<Chunk>> chunks;

    PNGImage(std::vector<std::vector<Pixel>>& data);

    void write(std::ostream& file);
};

namespace Chunks {

    struct IHDR : public Chunk {
        uint32_t width;
        uint32_t height;
        uint8_t bit_depth;
        uint8_t color_type;
        uint8_t compression_method;
        uint8_t filter_method;
        uint8_t interlace_method;

        IHDR(uint32_t width, uint32_t height) : Chunk(13, "IHDR"),
            width(width), height(height), bit_depth(16), color_type(6),
            compression_method(0), filter_method(0), interlace_method(0) {};

        void write_data(CrcStream& out) override;
    };

    struct IDAT : public Chunk {
        // to fill in later
        IDAT() : Chunk(0, "IDAT") {}
    };

    struct IEND : public Chunk {
        IEND() : Chunk(0, "IEND") {}
    };

    struct gAMA : public Chunk {
        uint32_t gamma;

        gAMA(uint32_t gamma) : Chunk(4, "gAMA"),
            gamma(gamma) {}

        void write_data(CrcStream& out) override;
    };

    struct cHRM : public Chunk {
        uint32_t white_point_x;
        uint32_t white_point_y;
        uint32_t red_x;
        uint32_t red_y;
        uint32_t green_x;
        uint32_t green_y;
        uint32_t blue_x;
        uint32_t blue_y;

        cHRM(uint32_t white_point_x, uint32_t white_point_y,
            uint32_t red_x, uint32_t red_y,
            uint32_t green_x, uint32_t green_y,
            uint32_t blue_x, uint32_t blue_y) : Chunk(32, "cHRM"),
            white_point_x(white_point_x), white_point_y(white_point_y),
            red_x(red_x), red_y(red_y),
            green_x(green_x), green_y(green_y),
            blue_x(blue_x), blue_y(blue_y) {}

        void write_data(CrcStream& out) override;
    };

    struct sRGB : public Chunk {
        enum class intent_t : uint8_t {
            perceptual = 0,
            relative_colormetric = 1,
            saturation = 2,
            absolute_colormetric = 3
        };

        intent_t rendering_intent;

        sRGB(intent_t rendering_intent) : Chunk(1, "sRGB"),
            rendering_intent(rendering_intent) {}

        void write_data(CrcStream& out) override;
    };
}
