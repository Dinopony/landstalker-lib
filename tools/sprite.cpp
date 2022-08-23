#include <cstdint>
#include "sprite.hpp"
#include "../exceptions.hpp"
#include "lodepng.h"
#include "lz77.hpp"
#include <iostream>

constexpr uint8_t TILE_SIZE_IN_BYTES = 32;

void Sprite::replace_color(uint8_t color_index, uint8_t new_color_index, size_t start_index, size_t end_index)
{
    for(size_t i=start_index ; i<end_index && i<_data.size() ; ++i)
    {
        uint8_t& byte = _data.at(i);
        uint8_t msc = byte >> 4;
        uint8_t lsc = byte & 0x0F;

        if(msc == color_index)
            msc = new_color_index;
        if(lsc == color_index)
            lsc = new_color_index;

        byte = (msc << 4) | lsc;
    }
}

void Sprite::replace_color_in_tile(uint8_t color_index, uint8_t new_color_index, uint8_t tile_index)
{
    this->replace_color(color_index,
                        new_color_index,
                        tile_index * TILE_SIZE_IN_BYTES,
                        (tile_index+1) * TILE_SIZE_IN_BYTES);
}

uint8_t Sprite::get_pixel(uint8_t tile_id, uint8_t x, uint8_t y)
{
    size_t pixel_index = ((tile_id * 64) + (y*8) + x);
    size_t byte_index = pixel_index / 2;
    bool lsb = pixel_index % 2;

    if(lsb)
        return _data.at(byte_index) & 0x0F;
    else
        return (_data.at(byte_index) & 0xF0) >> 4;
}

void Sprite::set_pixel(uint8_t tile_id, uint8_t x, uint8_t y, uint8_t color)
{
    size_t pixel_index = ((tile_id * 64) + (y*8) + x);
    size_t byte_index = pixel_index / 2;
    bool lsb = pixel_index % 2;

    uint8_t byte = _data.at(byte_index);
    if(lsb)
    {
        byte &= 0xF0;
        byte |= color;
        _data[byte_index] = byte;
    }
    else
    {
        byte &= 0x0F;
        byte |= (color << 4);
        _data[byte_index] = byte;
    }
}

static uint8_t read_byte_from(const uint8_t* it)
{
    return *it;
}

static uint16_t read_word_from(const uint8_t* it)
{
    uint8_t msb = read_byte_from(it);
    it += 1;
    uint8_t lsb = read_byte_from(it);
    return (msb << 8) | lsb;
}

ByteArray Sprite::encode()
{
    constexpr size_t THRESHOLD_FOR_GROUPING_ZEROES = 8;

    if(_data.size() > 0xFFF)
        throw LandstalkerException("Trying to encode a sprite without compression that is above 0xFFF bytes in size");

    ByteArray encoded_bytes;
    for(const SubSpriteMetadata& subsprite : _subsprites)
        encoded_bytes.add_word(subsprite.to_word());

    ByteArray current_sequence;
    uint16_t zeroes_buffer = 0;
    for(size_t index_in_data = 0; index_in_data < _data.size() ; index_in_data += 0x2)
    {
        uint16_t current_word = read_word_from(&_data[index_in_data]);

        if(current_word == 0x0000)
        {
            zeroes_buffer += 1;
            continue;
        }

        if(zeroes_buffer > 0)
        {
            if(zeroes_buffer >= THRESHOLD_FOR_GROUPING_ZEROES)
            {
                // This zeroes sequence is big enough to be encoded separately, do it
                if(!current_sequence.empty())
                {
                    // If there was a pending sequence before encountering those zeroes, commit it now
                    encoded_bytes.add_word(0x0000 | (current_sequence.size() / 2));
                    encoded_bytes.add_bytes(current_sequence);
                    current_sequence.clear();
                }

                // End and commit the current zero collection sequence
                encoded_bytes.add_word(0x8000 | zeroes_buffer);
            }
            else
            {
                // This zeroes sequence is not big enough to be encoded separately, include it inside the current sequence
                for(uint16_t i=0 ; i<zeroes_buffer ; ++i)
                    current_sequence.add_word(0x0000);
            }

            zeroes_buffer = 0;
        }

        current_sequence.add_word(current_word);
    }


    if(zeroes_buffer >= THRESHOLD_FOR_GROUPING_ZEROES)
    {
        // This zeroes sequence is big enough to be encoded separately, do it
        if(!current_sequence.empty())
        {
            // If there was a pending sequence before encountering those zeroes, commit it now
            encoded_bytes.add_word(0x0000 | (current_sequence.size() / 2));
            encoded_bytes.add_bytes(current_sequence);
        }

        // End and commit the current zero collection sequence
        encoded_bytes.add_word(0x8000 | 0x4000 | zeroes_buffer);
    }
    else
    {
        // This zeroes sequence is not big enough to be encoded separately, include it inside the current sequence
        // and commit it
        for(uint16_t i=0 ; i<zeroes_buffer ; ++i)
            current_sequence.add_word(0x0000);

        if(!current_sequence.empty())
        {
            encoded_bytes.add_word(0x4000 | (current_sequence.size() / 2));
            encoded_bytes.add_bytes(current_sequence);
        }
    }

    return encoded_bytes;
}

Sprite Sprite::decode_from(const uint8_t* it)
{
    size_t bytes_prediction = 0;
    std::vector<SubSpriteMetadata> subsprites;
    while(true)
    {
        SubSpriteMetadata subsprite(read_word_from(it));
        it += 2;
        subsprites.emplace_back(subsprite);

        bytes_prediction += (subsprite.tile_count_w * subsprite.tile_count_h * TILE_SIZE_IN_BYTES);

        if(subsprite.last_subsprite)
            break;
    }

    std::vector<uint8_t> result_bytes;
    result_bytes.reserve(bytes_prediction);
    uint8_t ctrl = 0;
    while((ctrl & 0x04) == 0)
    {
        uint16_t command = read_word_from(it);
        it += 2;

        ctrl = command >> 12;
        uint16_t count = command & 0x0FFF;

        if (ctrl & 0x08)
        {
            // Insert X zero words
            for(size_t i=0 ; i<count*2 ; ++i)
                result_bytes.emplace_back(0x00);
        }
        else if (ctrl & 0x02)
        {
            // Read X compressed bytes
            std::vector<uint8_t> decompressed = decode_lz77(it);
            result_bytes.insert(result_bytes.end(), decompressed.begin(), decompressed.end());
        }
        else
        {
            // Copy X raw words
            for(size_t i=0 ; i<count*2 ; ++i)
            {
                result_bytes.emplace_back(read_byte_from(it));
                it += 1;
            }
        }
    }

    return { result_bytes, subsprites };
}

void Sprite::write_to_png(const std::string& path, const ColorPalette<16>& palette)
{
    int origin_x = INT32_MAX;
    int origin_y = INT32_MAX;
    int highest_x = 0;
    int highest_y = 0;
    for(auto& subsprite : _subsprites)
    {
        if(subsprite.x < origin_x)
            origin_x = (int)subsprite.x;
        if(subsprite.y < origin_y)
            origin_y = (int)subsprite.y;

        if(subsprite.x + (subsprite.tile_count_w * 8) > highest_x)
            highest_x = subsprite.x + (subsprite.tile_count_w * 8);
        if(subsprite.y + (subsprite.tile_count_h * 8) > highest_y)
            highest_y = subsprite.y + (subsprite.tile_count_h * 8);
    }

    unsigned int width = highest_x - origin_x;
    unsigned int height = highest_y - origin_y;

    // The image has width * height RGBA pixels (each one composed of 4 bytes : R G B A)
    std::vector<uint8_t> image_data;
    image_data.resize(width * height * 4, 0);

    size_t current_tile_index = 0;
    for(auto& subsprite : _subsprites)
    {
        unsigned int global_subsprite_origin_x = subsprite.x - origin_x;
        unsigned int global_subsprite_origin_y = subsprite.y - origin_y;

        for(size_t tile_y = 0 ; tile_y < subsprite.tile_count_h ; ++tile_y)
        {
            for(size_t tile_x = 0 ; tile_x < subsprite.tile_count_w ; ++tile_x)
            {
                size_t tile_index = (tile_x * subsprite.tile_count_h) + tile_y + current_tile_index;
                for(size_t x=0 ; x<8 ; ++x)
                {
                    for(size_t y = 0 ; y < 8 ; ++y)
                    {
                        uint8_t color_index = this->get_pixel(tile_index, x, y);
                        Color color = palette[color_index];

                        size_t global_x = global_subsprite_origin_x + (tile_x * 8) + x;
                        size_t global_y = global_subsprite_origin_y + (tile_y * 8) + y;
                        size_t pixel_index = (global_y * width + global_x) * 4;

                        image_data[pixel_index] = color.r();
                        image_data[pixel_index+1] = color.g();
                        image_data[pixel_index+2] = color.b();
                        image_data[pixel_index+3] = color_index ? 0xFF : 0x00;
                    }
                }
            }
        }
        current_tile_index += subsprite.tile_count_h * subsprite.tile_count_w;
    }

    lodepng::State state;
    lodepng_state_init(&state);
    state.encoder.auto_convert = false;
    state.info_png.color.colortype = LCT_PALETTE;
    size_t i=0;
    for(Color c : palette)
        lodepng_palette_add(&state.info_png.color, c.r(), c.g(), c.b(), (i++) ? 0xFF : 0x00);

    std::vector<uint8_t> encoded_bytes;
    unsigned int error = lodepng::encode(encoded_bytes, image_data, width, height, state);
    if(error)
        std::cout << "PNG encoding error " << error << ": "<< lodepng_error_text(error) << std::endl;

    error = lodepng::save_file(encoded_bytes, path);
    if(error)
        std::cout << "PNG saving error " << error << ": "<< lodepng_error_text(error) << std::endl;
}
