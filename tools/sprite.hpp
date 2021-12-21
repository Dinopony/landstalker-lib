#pragma once

#include <cstdint>
#include <vector>
#include "byte_array.hpp"
#include "../md_tools.hpp"

struct SubSpriteMetadata {
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t tile_count_w = 0;
    uint8_t tile_count_h = 0;
    uint16_t original_word = 0;
    bool last_subsprite = false;

    explicit SubSpriteMetadata(uint16_t word)
    {
        original_word = word;

        uint8_t byte_1 = word >> 8;
        uint8_t byte_2 = word & 0x00FF;

        y = (byte_1 & 0x7C) << 1;
        x = (byte_2 & 0x7C) << 1;
        tile_count_w = (byte_1 & 0x03) + 1;
        tile_count_h = (byte_2 & 0x03) + 1;
//        if (y > 0x80) y -= 0x100;
//        if (x > 0x80) x -= 0x100;

        if(byte_2 & 0x80)
            last_subsprite = true;
    }

    [[nodiscard]] uint16_t to_word() const { return original_word; }
};

class Sprite
{
private:
    std::vector<uint8_t> _data;
    std::vector<SubSpriteMetadata> _subsprites;

public:
    Sprite(std::vector<uint8_t> data, std::vector<SubSpriteMetadata> subsprites) :
            _data       (std::move(data)),
            _subsprites (std::move(subsprites))
    {}

    Sprite(const uint8_t* data, size_t data_size, std::vector<SubSpriteMetadata> subsprites) :
            _data       (data, data + data_size),
            _subsprites (std::move(subsprites))
    {}

    void replace_color(uint8_t color_index, uint8_t new_color_index) { this->replace_color(color_index, new_color_index, 0, _data.size()); }
    void replace_color(uint8_t color_index, uint8_t new_color_index, size_t start_index, size_t end_index);
    void replace_color_in_tile(uint8_t color_index, uint8_t new_color_index, uint8_t tile_index);

    void set_pixel(uint8_t tile_id, uint8_t x, uint8_t y, uint8_t color);

    [[nodiscard]] const std::vector<uint8_t>& data() const { return _data; }
    [[nodiscard]] const std::vector<SubSpriteMetadata>& subsprites() const { return _subsprites; }

    ByteArray encode();

    static Sprite decode_from(const uint8_t* it);
};

