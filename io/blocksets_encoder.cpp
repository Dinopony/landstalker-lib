#include "io.hpp"

#include "../model/blockset.hpp"
#include "../tools/tile_queue.hpp"
#include "../tools/byte_array.hpp"
#include "../tools/bitstream_writer.hpp"

static uint16_t encode_tile(uint16_t tile_index, BitstreamWriter& bitstream, TileQueue& tile_queue)
{
    int index_in_queue = tile_queue.find(tile_index);
    if(index_in_queue >= 0)
    {
        bitstream.add_bit(true);
        bitstream.add_number(index_in_queue, 4);
        tile_queue.move_to_front(index_in_queue);
    }
    else
    {
        bitstream.add_bit(false);
        bitstream.add_number(tile_index, 11);
        tile_queue.push_front(tile_index);
    }

    return tile_queue.front();
}

static std::vector<uint8_t> pack_tiles(const std::vector<Tile>& tiles, BitstreamWriter& bitstream)
{
    TileQueue tile_queue(16);

    // Encode tile indexes by pairs, using a memory queue with the 16 last results
    for(size_t i=0 ; i < tiles.size() ; i += 2)
    {
        uint16_t first_tile_index = tiles[i].tile_index();
        uint16_t second_tile_index = tiles[i+1].tile_index();

        encode_tile(first_tile_index, bitstream, tile_queue);

        bool is_next_tile_in_tileset;
        if(tiles[i].attribute(Tile::ATTR_HFLIP))
            is_next_tile_in_tileset = (second_tile_index == first_tile_index - 1);
        else
            is_next_tile_in_tileset = (second_tile_index == first_tile_index + 1);

        if(is_next_tile_in_tileset)
            bitstream.add_bit(true);
        else
        {
            bitstream.add_bit(false);
            encode_tile(second_tile_index, bitstream, tile_queue);
        }
    }

    return bitstream.bytes();
}

static std::vector<uint8_t> pack_tile_attributes(const std::vector<Tile>& tiles, BitstreamWriter& bitstream, uint16_t attr)
{
    bool current_attribute_value = false;
    bool firstLoop = true;
    size_t current_position = 0;

    while(current_position < tiles.size())
    {
        uint32_t consecutive_count = 0;
        while(tiles[current_position].attribute(attr) == current_attribute_value)
        {
            consecutive_count += 1;
            current_position += 1;
            if(current_position == tiles.size())
                break;
        }

        if(firstLoop)
            firstLoop = false;
        else
            consecutive_count -= 1;

        bitstream.add_variable_length_number(consecutive_count);
        current_attribute_value = !current_attribute_value;
    }

    return bitstream.bytes();
}

ByteArray io::encode_blockset(Blockset* blockset)
{
    BitstreamWriter bitstream;

    std::vector<Tile> tiles;
    for(const auto& block : blockset->blocks())
        for(const Tile& tile : block)
            tiles.emplace_back(tile);

    pack_tile_attributes(tiles, bitstream, Tile::ATTR_PRIORITY);
    pack_tile_attributes(tiles, bitstream, Tile::ATTR_VFLIP);
    pack_tile_attributes(tiles, bitstream, Tile::ATTR_HFLIP);

    pack_tiles(tiles, bitstream);

    ByteArray blockset_bytes;
    blockset_bytes.add_word(blockset->blocks().size());
    blockset_bytes.add_bytes(bitstream.bytes());
    return blockset_bytes;
}
