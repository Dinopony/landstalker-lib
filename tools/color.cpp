#include "color.hpp"

#include <cmath>
#include "./tools.hpp"
#include "../exceptions.hpp"

Color::Color(uint8_t r, uint8_t g, uint8_t b)
{
    this->rgb(r,g,b);
}

Color::Color(const std::string& str)
{
    uint16_t r, g, b;
    if(str.size() == 4 && str.starts_with('#'))
    {
        // Format 1 : "#RGB"
        std::stringstream ss;
        ss << std::hex << str[1];
        ss >> r;
        ss.clear();
        ss << std::hex << str[2];
        ss >> g;
        ss.clear();
        ss << std::hex << str[3];
        ss >> b;

        this->rgb((uint8_t)r << 4, (uint8_t)g << 4, (uint8_t)b << 4);
    }
    else if(str.size() == 7 && str.starts_with('#'))
    {
        // Format 2 : "#RRGGBB"
        std::stringstream ss;
        ss << std::hex << str[1] << str[2];
        ss >> r;
        ss.clear();
        ss << std::hex << str[3] << str[4];
        ss >> g;
        ss.clear();
        ss << std::hex << str[5] << str[6];
        ss >> b;

        this->rgb((uint8_t)r, (uint8_t)g, (uint8_t)b);
    }
    else
    {
        throw LandstalkerException("Could not parse Color from string '" + str + "'. "
                                   "Valid formats are \"#RGB\" and \"#RRGGBB\"");
    }
}

void Color::rgb(uint8_t r, uint8_t g, uint8_t b)
{
    _r = r;
    _g = g;
    _b = b;
    _invalid = false;
}

Color Color::multiply(double factor) const
{
    double r = std::min(_r * factor, 255.0);
    uint8_t rounded_r = (uint8_t)std::round(r);

    double g = std::min(_g * factor, 255.0);
    uint8_t rounded_g = (uint8_t)std::round(g);

    double b = std::min(_b * factor, 255.0);
    uint8_t rounded_b = (uint8_t)std::round(b);

    return { rounded_r, rounded_g, rounded_b };
}

Color Color::desaturate(double factor) const
{
    double gray_equivalent = (_r + _g + _b) / 3.0;

    double r = (_r * (1-factor)) + (gray_equivalent * factor);
    uint8_t rounded_r = (uint8_t)std::round(r);

    double g = (_g * (1-factor)) + (gray_equivalent * factor);
    uint8_t rounded_g = (uint8_t)std::round(g);

    double b = (_b * (1-factor)) + (gray_equivalent * factor);
    uint8_t rounded_b = (uint8_t)std::round(b);

    return { rounded_r, rounded_g, rounded_b };
}

Color Color::subtract(uint8_t value) const
{
    int32_t r = _r - value;
    if(r < 0)
        r = 0;

    int32_t g = _g - value;
    if(g < 0)
        g = 0;

    int32_t b = _b - value;
    if(b < 0)
        b = 0;

    return { (uint8_t)r, (uint8_t)g, (uint8_t)b };
}

bool Color::operator==(const Color& other) const
{
    return _r == other._r
        && _g == other._g
        && _b == other._b
        && _invalid == other._invalid;
}

[[nodiscard]] uint16_t Color::to_bgr_word() const
{
    if(_invalid)
        return 0xFFFF;

    uint16_t color_word = 0;
    color_word |= _r >> 4;
    color_word |= (_g >> 4) << 4;
    color_word |= (_b >> 4) << 8;
    return color_word;
}

Color Color::from_bgr_word(uint16_t word)
{
    uint8_t r = (word & 0xF) << 4;
    uint8_t g = ((word >> 4) & 0xF) << 4;
    uint8_t b = ((word >> 8) & 0xF) << 4;
    return {r, g, b};
}

[[nodiscard]] Json Color::to_json() const
{
    return { { "r", _r }, { "g", _g }, { "b", _b } };
}

Color Color::from_json(const Json& json)
{
    if(json.is_object())
    {
        uint8_t r = json.at("r");
        uint8_t g = json.at("g");
        uint8_t b = json.at("b");
        return { r, g, b };
    }
    else if(json.is_array())
    {
        uint8_t r = json[0];
        uint8_t g = json[1];
        uint8_t b = json[2];
        return { r, g, b };
    }

    return { std::string(json) };
}
