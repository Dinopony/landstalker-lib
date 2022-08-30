#include "io.hpp"
#include "../tools/byte_array.hpp"
#include "../model/map_layout.hpp"

static ByteArray encode_data_block(const std::vector<uint16_t>& data_block, uint16_t default_value)
{
    // Remove trailing default values
    std::vector<uint16_t> tile_values = data_block;
    while(!tile_values.empty() && *tile_values.rbegin() == default_value)
        tile_values.pop_back();

    ByteArray bytes;
    uint32_t current_chain_size = 0;
    uint16_t current_chain_word = 0x0000;
    auto output_chain = [&]()
    {
        while(current_chain_size > 1)
        {
            if(current_chain_word == default_value)
            {
                // 0 -> "1000AAAA AAAAAAAA" case: skip A words
                uint16_t effective_size = std::min(current_chain_size, (uint32_t)0x0FFF);
                bytes.add_word(0x8000 + effective_size);
                current_chain_size -= effective_size;
            }
            else if(current_chain_word <= 0x00FF)
            {
                // 2 -> "1010AAAA BBBBBBBB" case: repeat A times byte B
                uint16_t effective_size = std::min(current_chain_size, (uint32_t)0x000F);
                bytes.add_word(0xA000 + (effective_size << 8) + current_chain_word);
                current_chain_size -= effective_size;
            }
            else if((current_chain_word & 0xFF00) == current_chain_word)
            {
                // 5 -> "1101BBBB BBBBAAAA" case: repeat A+2 times byte B as an inverted word (0x26 >>> 0x2600)
                uint16_t effective_size = std::min(current_chain_size, (uint32_t)17);
                bytes.add_word(0xD000 + (current_chain_word >> 4) + (effective_size - 2));
                current_chain_size -= effective_size;
            }
            else if(current_chain_word <= 0x3FF)
            {
                // 6 -> "1110WWWW WWWWWWXX" case: repeat 2+X times word W
                uint16_t effective_size = std::min(current_chain_size, (uint32_t)5);
                bytes.add_word(0xE000 + (current_chain_word << 2) + (effective_size - 2));
                current_chain_size -= effective_size;
            }
            else if(current_chain_size > 2)
            {
                // 3 -> "1011AAAA AAAAAAAA" case: repeat next word A times
                uint16_t effective_size = std::min(current_chain_size, (uint32_t)0x0FFF);
                bytes.add_word(0xB000 + effective_size);
                bytes.add_word(current_chain_word);
                current_chain_size -= effective_size;
            }
            else break;

            // 4, 7 are free
            // 1100, 1111
        }

        while(current_chain_size > 0)
        {
            bytes.add_word(current_chain_word);
            current_chain_size--;
        }
    };

    for(uint16_t tile : tile_values)
    {
        if(tile == current_chain_word && current_chain_size > 0)
        {
            current_chain_size++;
            continue;
        }

        output_chain();
        current_chain_word = tile;
        current_chain_size = 1;
    }

    output_chain();
    bytes.add_word(0x8000);

    // =========== Post processing ==============
    // Pack successive increasing values, etc...

    uint16_t last_word;
    uint16_t current_word = bytes.word_at(0);
    for(size_t i=2 ; i<bytes.size() ; i+=2)
    {
        last_word = current_word;
        current_word = bytes.word_at(i);

        bool is_last_word_operand = (last_word & 0x8000);
        bool is_current_word_operand = (current_word & 0x8000);
        if(is_current_word_operand || is_last_word_operand)
            continue;

/*        if(current_word == last_word + 1 && last_word <= 0x3FF)
        {
            // "1100WWWW WWWWWWXX" case: repeat word W 2+X times while incrementing value on every write
            size_t x = 0;
            size_t forward_cursor = i+2;
            while(forward_cursor < bytes.size() && x < 3)
            {
                uint16_t next_byte = bytes.word_at(forward_cursor);
                if(next_byte & 0x8000)
                    break;
                if(next_byte != current_word + 1 + x)
                    break;
                ++x;
                bytes.remove_word(forward_cursor);
            }
            current_word = 0xC000 + (last_word << 2) + x;
            bytes.remove_word(i);
            i -= 2;
            bytes.word_at(i, current_word);
        }
        else*/if(current_word <= 0x3F && last_word <= 0x3F)
        {
            // 1 -> "1001AAAA AABBBBBB" case: place byte A then byte B as words
            current_word = 0x9000 + (last_word << 6) + current_word;
            bytes.remove_word(i);
            i -= 2;
            bytes.word_at(i, current_word);
        }
    }

    return bytes;
}

ByteArray io::encode_map_layout(MapLayout* layout)
{
    ByteArray data;

    data.add_byte(layout->top()); // = 9 = amount of big chunks to skip
    data.add_byte(layout->left()); // = 8 = amount of small words to skip
    data.add_byte(layout->width()); // = 14 = words to copy for each line (before going to next chunk)
    data.add_byte(layout->heightmap_width());

    data.add_bytes(encode_data_block(layout->foreground_tiles(), 0x0000));
    data.add_bytes(encode_data_block(layout->background_tiles(), 0x0000));

//    std::vector<uint16_t> swapped_heightmap = layout->heightmap();
//    for(size_t i=0 ; i<swapped_heightmap.size() ; ++i)
//    {
//        uint16_t value = swapped_heightmap[i];
//        uint16_t flag = value & 0xF000;
//        uint16_t height = value & 0x0F00;
//        uint16_t floor_type = value & 0x00FF;
//        swapped_heightmap[i] = (flag >> 8) + (height >> 8) + (floor_type << 16);
//    }
//    data.add_bytes(encode_data_block(swapped_heightmap, 0x0040));

    data.add_bytes(encode_data_block(layout->heightmap(), 0x4000));

    return data;
}
