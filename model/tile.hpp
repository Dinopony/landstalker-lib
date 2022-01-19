#pragma once

#include <cstdint>
#include "../tools/json.hpp"

class Tile {
private:
    uint16_t _tile_index = 0;
    uint16_t _attributes = 0;

public:
    Tile() = default;

    void tile_index(uint16_t tile_index) { _tile_index = tile_index; }
    [[nodiscard]] uint16_t tile_index() const { return _tile_index; }

    void attribute(uint16_t attr, bool value)
    {
        if(value)
            _attributes |= attr;
        else
            _attributes &= ~attr;
    }
    [[nodiscard]] bool attribute(uint16_t attr) const { return _attributes & attr; }

    [[nodiscard]] Json to_json() const
    {
        std::string attributes;
        if(attribute(ATTR_PRIORITY))
            attributes.push_back('p');
        if(attribute(ATTR_HFLIP))
            attributes.push_back('h');
        if(attribute(ATTR_VFLIP))
            attributes.push_back('v');

        if(attributes.empty())
            return { _tile_index };
        else
            return { _tile_index, attributes };
    }

    static constexpr uint16_t ATTR_PRIORITY = 0x8000;
    static constexpr uint16_t ATTR_VFLIP = 0x1000;
    static constexpr uint16_t ATTR_HFLIP = 0x0800;
};
