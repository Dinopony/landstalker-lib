#pragma once

#include <array>
#include <cstdint>
#include "color.hpp"
#include "json.hpp"

template <size_t N>
class ColorPalette : public std::array<Color, N>
{
public:
    void clear()
    {
        for(Color& color : *this)
            color = Color();
    }

    [[nodiscard]] bool is_valid() const
    {
        for(const Color& color : *this)
            if(color.invalid())
                return false;
        return true;
    }

    void multiply(double factor)
    {
        for(Color& color : *this)
            color = color.multiply(factor);
    }

    void desaturate(double factor)
    {
        for(Color& color : *this)
            color = color.desaturate(factor);
    }

    void subtract(uint8_t value)
    {
        for(Color& color : *this)
            color = color.subtract(value);
    }

    [[nodiscard]] Json to_json() const
    {
        Json json = Json::array();
        for(const Color& color : *this)
            json.emplace_back(color.to_json());
        return json;
    }
};

using MapPalette = ColorPalette<13>;
using EntityLowPalette = ColorPalette<6>;
using EntityHighPalette = ColorPalette<7>;