#include "png.hpp"

#include <numeric>
#include <cmath>
#include <iostream>
#include <type_traits>
#include <algorithm>
#include <codecvt>

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

CrcStream CrcStream::operator<<(std::string data) {
    for (uint8_t c : data) {
        this->operator<<(c);
    }
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
    write_data(stream);

    WriteBigEndian(file, stream.get_crc());
}

void Chunks::IHDR::write_data(CrcStream& out) {
    WriteBigEndian(out, width);
    WriteBigEndian(out, height);
    out << bit_depth
        << color_type
        << compression_method
        << filter_method
        << interlace_method;
}

void Chunks::gAMA::write_data(CrcStream& out) {
    WriteBigEndian(out, gamma);
}

void Chunks::cHRM::write_data(CrcStream& out) {
    WriteBigEndian(out, white_point_x);
    WriteBigEndian(out, white_point_y);
    WriteBigEndian(out, red_x);
    WriteBigEndian(out, red_y);
    WriteBigEndian(out, green_x);
    WriteBigEndian(out, green_y);
    WriteBigEndian(out, blue_x);
    WriteBigEndian(out, blue_y);
}

template <typename Base, typename Underlying = std::underlying_type_t<Base>>
static Underlying to_underlying_type(Base b) {
    return static_cast<Underlying>(b);
}

void Chunks::sRGB::write_data(CrcStream& out) {
    out << to_underlying_type(rendering_intent);
}

namespace Chunks {
    namespace keywords {
        std::string title("Title");
        std::string author("Author");
        std::string description("Description");
        std::string copyright("Copyright");
        std::string creation_time("Creation Time");
        std::string software("Software");
        std::string disclaimer("Disclaimer");
        std::string warning("Warning");
        std::string source("Source");
        std::string comment("Comment");
    };
};

bool isInvalidLatin1(uint8_t c) {
    return (c < 0x20 && c != '\n') || (c >= 0x7f && c <= 0x9F);
}

bool isInvalidUTF8(uint8_t c) {
    return (c < 0x20 && c != '\n') || (c == 0xC0) || (c == 0xC1) || (c >= 0xF5);
}

void filter_str(std::string& str, bool (*pred)(uint8_t)) {
    str.erase(std::remove_if(str.begin(), str.end(), pred), str.end());
}

Chunks::tEXt::tEXt(std::string keyword, std::string text) : Chunk(0, "tEXt") {
    if (keyword.size() > 79) {
        keyword = keyword.substr(0, 79);
        std::cerr << "Truncating Keyword\n";
    }

    filter_str(keyword, isInvalidLatin1);
    filter_str(text, isInvalidLatin1);
    

    this->keyword = keyword;
    this->text = text;
    this->length = static_cast<uint32_t>(keyword.size() + 1 + text.size());
}

void Chunks::tEXt::write_data(CrcStream& out) {
    out << keyword << '\0' << text;
}

Chunks::iTXt::iTXt(std::string keyword, std::string text, 
    std::string language, std::string translated) : Chunk(0, "iTXt"),
    compression_flag(0), compression_method(0) {

    if (keyword.size() > 79) {
        keyword = keyword.substr(0, 79);
        std::cerr << "Truncating Keyword\n";
    }

    filter_str(keyword, isInvalidUTF8);
    filter_str(text, isInvalidUTF8);
    filter_str(language, isInvalidUTF8);
    filter_str(translated, isInvalidUTF8);

    this->keyword = keyword;
    this->text = text;
    this->language = language;
    this->translated_keyword = translated;
    this->length = static_cast<uint32_t>(
        keyword.size() + 3 + language.size() + 1 + translated.size() + 1 + text.size());
}

void Chunks::iTXt::write_data(CrcStream& out) {
    out << keyword << '\0' 
        << compression_flag << compression_method 
        << language << '\0' 
        << translated_keyword << '\0' 
        << text;
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

    chunks.push_back(std::make_unique<Chunks::sRGB>(Chunks::sRGB::intent_t::saturation));
    chunks.push_back(std::make_unique<Chunks::gAMA>(45455));
    chunks.push_back(std::make_unique<Chunks::cHRM>(
        31270, 32900, 64000, 33000, 30000, 60000, 15000, 6000));
}


void PNGImage::meta(std::string data, std::string keyword, std::string language, std::string translated) {
    if (std::any_of(data.begin(), data.end(), [](uint8_t c) {
        return c > 0x7F;
    })) {
        chunks.push_back(std::make_unique<Chunks::iTXt>(keyword, data, language, translated));
    } else {
        chunks.push_back(std::make_unique<Chunks::tEXt>(keyword, data));
    }
}

void PNGImage::title(std::string data, std::string language, std::string translated) {
    meta(data, Chunks::keywords::title, language, translated);
}

void PNGImage::author(std::string data, std::string language, std::string translated) {
    meta(data, Chunks::keywords::author, language, translated);
}

void PNGImage::description(std::string data, std::string language, std::string translated) {
    meta(data, Chunks::keywords::description, language, translated);
}

void PNGImage::copyright(std::string data, std::string language, std::string translated) {
    meta(data, Chunks::keywords::copyright, language, translated);
}

void PNGImage::creation_time(std::string data, std::string language, std::string translated) {
    meta(data, Chunks::keywords::creation_time, language, translated);
}

void PNGImage::software(std::string data, std::string language, std::string translated) {
    meta(data, Chunks::keywords::software, language, translated);
}

void PNGImage::disclaimer(std::string data, std::string language, std::string translated) {
    meta(data, Chunks::keywords::disclaimer, language, translated);
}

void PNGImage::warning(std::string data, std::string language, std::string translated) {
    meta(data, Chunks::keywords::warning, language, translated);
}

void PNGImage::source(std::string data, std::string language, std::string translated) {
    meta(data, Chunks::keywords::source, language, translated);
}

void PNGImage::write(std::ostream& file) {
    if (hasError) return;

    chunks.push_back(std::make_unique<Chunks::IEND>());

    std::cout << "chunk count: " << chunks.size() << "\n";
    file << "\211PNG\r\n\032\n";

    for (auto& chunk : chunks) {
        chunk->write(file);
    }
}
