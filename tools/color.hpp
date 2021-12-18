#pragma once

#include <cstdint>
#include "json.hpp"

class Color
{
private:
    uint8_t _r = 0;
    uint8_t _g = 0;
    uint8_t _b = 0;
    bool _invalid = true;

public:
    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b);
    Color(const std::string& str);

    [[nodiscard]] uint8_t r() const { return _r; }
    void r(uint8_t r) { _r = r; _invalid = false; }

    [[nodiscard]] uint8_t g() const { return _g; }
    void g(uint8_t g) { _g = g; _invalid = false; }

    [[nodiscard]] uint8_t b() const { return _b; }
    void b(uint8_t b) { _b = b; _invalid = false; }

    void rgb(uint8_t r, uint8_t g, uint8_t b);

    [[nodiscard]] bool invalid() const { return _invalid; }
    void invalid(bool invalid) { _invalid = invalid; }

    [[nodiscard]] Color multiply(double factor) const;
    [[nodiscard]] Color desaturate(double factor) const;
    [[nodiscard]] Color subtract(uint8_t value) const;

    bool operator==(const Color& other) const;

    [[nodiscard]] uint16_t to_bgr_word() const;
    static Color from_bgr_word(uint16_t word);

    [[nodiscard]] Json to_json() const;
    static Color from_json(const Json& json);
};






