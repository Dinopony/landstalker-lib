#include <cstdint>
#include "sprite.hpp"
#include "../exceptions.hpp"
#include "lz77.hpp"

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
