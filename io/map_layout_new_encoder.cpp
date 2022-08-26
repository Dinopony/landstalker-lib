#include "io.hpp"
#include "../tools/byte_array.hpp"
#include "../model/map_layout.hpp"

static ByteArray map_foreground_as_bytes(MapLayout* layout)
{
    std::vector<uint16_t> tile_values = layout->foreground_tiles();
    while(!tile_values.empty() && *tile_values.rbegin() == 0x0000)
        tile_values.pop_back();

    ByteArray bytes;
    uint16_t null_chain_size = 0;
    for(uint16_t tile : tile_values)
    {
        if(tile == 0x0000)
            null_chain_size += 1;
        else
        {
            if(null_chain_size > 0)
            {
                bytes.add_word(0x8000 + null_chain_size);
                null_chain_size = 0;
            }
            bytes.add_word(tile);
        }
    }

    bytes.add_word(0xFFFF);
    return bytes;
}

static ByteArray map_background_as_bytes(MapLayout* layout)
{
    std::vector<uint16_t> tile_values = layout->background_tiles();
    while(!tile_values.empty() && *tile_values.rbegin() == 0x0000)
        tile_values.pop_back();

    ByteArray bytes;
    uint16_t null_chain_size = 0;
    for(uint16_t tile : tile_values)
    {
        if(tile == 0x0000)
            null_chain_size += 1;
        else
        {
            if(null_chain_size > 0)
            {
                bytes.add_word(0x8000 + null_chain_size);
                null_chain_size = 0;
            }
            bytes.add_word(tile);
        }
    }

    bytes.add_word(0xFFFF);
    return bytes;
}

static ByteArray map_heightmap_as_bytes(MapLayout* layout)
{
    std::vector<uint16_t> tile_values = layout->heightmap();
    while(!tile_values.empty() && *tile_values.rbegin() == 0x4000)
        tile_values.pop_back();

    ByteArray bytes;
    uint16_t null_chain_size = 0;
    for(uint16_t tile : tile_values)
    {
        if(tile == 0x4000)
            null_chain_size += 1;
        else
        {
            if(null_chain_size > 0)
            {
                bytes.add_word(0x8000 + null_chain_size);
                null_chain_size = 0;
            }
            bytes.add_word(tile);
        }
    }

    bytes.add_word(0xFFFF);
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

    data.add_bytes(map_foreground_as_bytes(layout));
    data.add_bytes(map_background_as_bytes(layout));
    data.add_bytes(map_heightmap_as_bytes(layout));

    return data;
}
