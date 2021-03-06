#pragma once
#define _CRT_SECURE_NO_WARNINGS
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

    Pixel() : r(0), g(0), b(0), a(0) {};

    Pixel(uint16_t r, uint16_t g, uint16_t b, uint16_t a = 0) :
        r(r), g(g), b(b), a(a) {};

    static Pixel zero_one(double r, double g, double b);

    // hsv conversion
    static Pixel HSV(double H, double S, double V);
};

class CrcStream {
    uint32_t crc;
    std::ostream& stream;

public:
    CrcStream(std::ostream& s) : crc(0), stream(s) {}

    CrcStream operator<<(uint8_t data);
    CrcStream operator<<(std::string data);

    uint32_t get_crc() {
        return ~crc;
    }
};

// assumptions
// - running on little endian machine
// - truecolor pixels
// - 16 bit samples
// - not interlaced
// - uses sRGB chunk (standard RGB)
//   - implies gAMA + cHRM
// - no iCCP chunk (advanced color management)
// - no sPLT chunk (suggested pallet)
// - no PLTE chunk (pallet)

struct PNGImage;
struct Chunk {
    uint32_t length; // max value = 2^31 - 1
    std::string type;
    uint32_t crc;

    Chunk(uint32_t length, std::string type, uint32_t crc = 0);

    void write(std::ostream& file, struct PNGImage& image);
    virtual void write_data(CrcStream& out, struct PNGImage& image) {};
    virtual void compute(struct PNGImage& image) {};
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

        void write_data(CrcStream& out, struct PNGImage& image) override;
    };

    struct IDAT : public Chunk {
        std::vector<std::vector<Pixel>> data;
        std::vector<uint8_t> bytes;
        int id;

        IDAT(std::vector<std::vector<Pixel>> data, int id) : Chunk(0, "IDAT"), 
            data(data), id(id) {}

        void compute(struct PNGImage& image) override;
        void write_data(CrcStream& out, struct PNGImage& image) override;
    };

    struct IEND : public Chunk {
        IEND() : Chunk(0, "IEND") {}
    };

    struct gAMA : public Chunk {
        uint32_t gamma;

        gAMA(uint32_t gamma) : Chunk(4, "gAMA"),
            gamma(gamma) {}

        void write_data(CrcStream& out, struct PNGImage& image) override;
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

        void write_data(CrcStream& out, struct PNGImage& image) override;
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

        void write_data(CrcStream& out, struct PNGImage& image) override;
    };

    namespace keywords {
        extern std::string title;
        extern std::string author;
        extern std::string description;
        extern std::string copyright;
        extern std::string creation_time;
        extern std::string software;
        extern std::string disclaimer;
        extern std::string warning;
        extern std::string source;
        extern std::string comment;
    };

    struct tEXt : public Chunk {
        std::string keyword;
        std::string text;

        tEXt(std::string keyword, std::string text);

        void write_data(CrcStream& out, struct PNGImage& image) override;
    };

    struct iTXt : public Chunk {
        std::string keyword;
        uint8_t compression_flag;
        uint8_t compression_method;
        std::string language;
        std::string translated_keyword;
        std::string text;

        iTXt(std::string keyword, std::string text, std::string language = "", std::string translated = "");

        void write_data(CrcStream& out, struct PNGImage& image) override;
    };

    struct tIME : public Chunk {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;

        tIME();

        void write_data(CrcStream& out, struct PNGImage& image) override;
    };

    struct bKGD : public Chunk {
        Pixel color;

        bKGD(Pixel color) : Chunk(6, "bKGD"),
            color(color) {}

        void write_data(CrcStream& out, struct PNGImage& image) override;
    };

    struct tRNS : public Chunk {
        Pixel color;

        tRNS(Pixel color) : Chunk(6, "tRNS"),
            color(color) {}

        void write_data(CrcStream& out, struct PNGImage& image) override;
    };
}

struct PNGImage {
    PNGImage();
    PNGImage(std::vector<std::vector<Pixel>>& data);

    void meta(std::string data, std::string keyword = Chunks::keywords::comment, std::string language = "", std::string translated = "");
    void title(std::string data, std::string language = "", std::string translated = "");
    void author(std::string data, std::string language = "", std::string translated = "");
    void description(std::string data, std::string language = "", std::string translated = "");
    void copyright(std::string data, std::string language = "", std::string translated = "");
    void creation_time(std::string data, std::string language = "", std::string translated = "");
    void software(std::string data, std::string language = "", std::string translated = "");
    void disclaimer(std::string data, std::string language = "", std::string translated = "");
    void warning(std::string data, std::string language = "", std::string translated = "");
    void source(std::string data, std::string language = "", std::string translated = "");

    void creation_time();
    void modification_time();

    void background(Pixel color);
    void transparent_color(Pixel color);
    void no_alpha();

    void bit_depth_8();

    void data(std::vector<std::vector<Pixel>> data);

    void write(std::ostream& file);

    bool use_alpha;
    bool use_8_bit;

private:
    bool has_error;
    bool has_background;
    int IDAT_count;

    std::vector<std::unique_ptr<Chunk>> chunks;
};
