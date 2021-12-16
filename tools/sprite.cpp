#include <vector>
#include <cstdint>
#include "sprite.hpp"
#include "../exceptions.hpp"

constexpr uint8_t TILE_SIZE_IN_BYTES = 32;

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

static std::vector<uint8_t> decode_lz77(const uint8_t*& it)
{
    std::vector<uint8_t> decompressed_bytes;
    uint8_t cmd;
    uint8_t cmd_remaining_bits = 0;

    while(true)
    {
        if(!cmd_remaining_bits)
        {
            cmd = read_byte_from(it);
            cmd_remaining_bits = 8;
            it += 1;
        }

        uint8_t next_cmd_bit = (cmd & 0x80);
        cmd <<= 1;
        cmd_remaining_bits -= 1;

        if(next_cmd_bit)
        {
            decompressed_bytes.emplace_back(read_byte_from(it));
            it += 1;
        }
        else
        {
            uint8_t byte_1 = read_byte_from(it);
            it += 1;
            uint8_t byte_2 = read_byte_from(it);
            it += 1;

            uint16_t offset = (byte_1 & 0xF0) << 4 | byte_2;
            uint8_t length = 18 - (byte_1 & 0x0F);
            if(offset)
            {
                while(length != 0)
                {
                    uint8_t byte_to_copy = decompressed_bytes[decompressed_bytes.size() - offset];
                    decompressed_bytes.emplace_back(byte_to_copy);
                    length -= 1;
                }
            }
            else break;
        }
    }

    return decompressed_bytes;
}

ByteArray Sprite::encode()
{
    if(_data.size() > 0xFFF)
        throw LandstalkerException("Trying to encode a sprite without compression that is above 0xFFF bytes in size");

    ByteArray encoded_bytes;
    for(const SubSpriteMetadata& subsprite : _subsprites)
        encoded_bytes.add_word(subsprite.to_word());

    uint16_t control = 0x4000 | (_data.size() / 2);
    encoded_bytes.add_word(control);
    encoded_bytes.add_bytes(_data);

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
