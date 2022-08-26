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
        // If we reach here, that means we broke the current chain if there was one
        while(current_chain_size > 1)
        {
            if(current_chain_word == default_value)
            {
                // "1000AAAA AAAAAAAA" case: skip A words
                uint16_t effective_size = std::min(current_chain_size, (uint32_t)0x0FFF);
                bytes.add_word(0x8000 + effective_size);
                current_chain_size -= effective_size;
            }
           //else if(current_chain_word <= 0x00FF)
           //{
           //    // "1010AAAA BBBBBBBB" case: repeat A times byte B
           //    uint16_t effective_size = std::min(current_chain_size, (uint32_t)0x000F);
           //    bytes.add_word(0xA000 + (effective_size << 8) + current_chain_word);
           //    current_chain_size -= effective_size;
           //}
            else
            {
                // "1011AAAA AAAAAAAA" case: repeat next word A times
                // TODO
                bytes.add_word(current_chain_word);
                current_chain_size--;
            }
        }

        // If the word is "lonely", just write it
        if(current_chain_size == 1)
            bytes.add_word(current_chain_word);
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
    return bytes;
}

ByteArray io::encode_map_layout(MapLayout* layout)
{
    ByteArray data;

    data.add_byte(layout->top()); // = 9 = amount of big chunks to skip
    data.add_byte(layout->left()); // = 8 = amount of small words to skip
    data.add_byte(layout->width()); // = 14 = words to copy for each line (before going to next chunk)
    data.add_byte(0xc); // heightmap top        // TODO: Remove, useless (fixed value)
    data.add_byte(0xc); // heightmap left       // TODO: Remove, useless (fixed value)
    data.add_byte(layout->heightmap_width());

    data.add_bytes(encode_data_block(layout->foreground_tiles(), 0x0000));
    data.add_bytes(encode_data_block(layout->background_tiles(), 0x0000));
    data.add_bytes(encode_data_block(layout->heightmap(), 0x4000));

    return data;
}
