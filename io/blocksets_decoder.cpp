#include "io.hpp"

#include "../model/blockset.hpp"
#include "../tools/bitstream.hpp"
#include "../tools/tile_queue.hpp"

static uint16_t decode_tile(Bitstream& bitstream, TileQueue& tile_queue)
{
    if(bitstream.next_bit())
    {
        uint32_t index = bitstream.read_bits(4);
        tile_queue.move_to_front(index);
    }
    else
    {
        uint32_t val = bitstream.read_bits(11);
        tile_queue.push_front(val);
    }

    return tile_queue.front();
}

static void read_tile_attributes(Bitstream& bitstream, std::vector<Tile>& tiles, uint16_t attr)
{
    bool current_attribute_value = false;
    bool firstLoop = true;

    size_t tile_id = 0;
    while(tile_id < tiles.size())
    {
        uint32_t num = bitstream.read_variable_length_number();
        if(firstLoop)
        {
            firstLoop = false;
            num -= 1;
        }

        size_t upper_bound = tile_id + num;
        for( ; tile_id < upper_bound ; ++tile_id)
            tiles[tile_id].attribute(attr, current_attribute_value);

        current_attribute_value = !current_attribute_value;
    }
}

static std::vector<Tile> decompress_tiles(Bitstream& bitstream, uint16_t tile_count)
{
    std::vector<Tile> tiles;
    tiles.resize(tile_count);

    // Apply VDP attributes to tiles before parsing indexes
    read_tile_attributes(bitstream, tiles, Tile::ATTR_PRIORITY);
    read_tile_attributes(bitstream, tiles, Tile::ATTR_VFLIP);
    read_tile_attributes(bitstream, tiles, Tile::ATTR_HFLIP);

    TileQueue tile_queue(16);

    // Decompress tile indexes by pairs, using a memory queue with the 16 last results
    for(size_t i=0 ; i < tiles.size() ; i += 2)
    {
        // Read first tile index
        uint16_t tile_index = decode_tile(bitstream, tile_queue);
        tiles[i].tile_index(tile_index);

        if(bitstream.next_bit())
        {
            // If next bit is set, next tile index depends on previous tile index
            if(tiles[i].attribute(Tile::ATTR_HFLIP))
                tiles[i+1].tile_index(tile_index - 1);
            else
                tiles[i+1].tile_index(tile_index + 1);
        }
        else
        {
            // If next bit is clear, next tile index is independant and must be read
            tiles[i+1].tile_index(decode_tile(bitstream, tile_queue));
        }
    }

    return tiles;
}

Blockset* io::decode_blockset(const md::ROM& rom, uint32_t addr)
{
    uint16_t block_count = rom.get_word(addr);
    Bitstream bitstream(rom.iterator_at(addr + 2));

    std::vector<Tile> tiles = decompress_tiles(bitstream, block_count*4);

    // Pack tiles into blocks
    std::vector<Blockset::Block> blocks;
    for(size_t i=0 ; i < tiles.size() ; i += 4)
        blocks.push_back({ tiles[i], tiles[i+1], tiles[i+2], tiles[i+3] });

    return new Blockset(blocks);
}
