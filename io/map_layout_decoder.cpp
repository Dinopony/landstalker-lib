#include "io.hpp"
#include "../tools/bitstream_reader.hpp"
#include "../model/map_layout.hpp"

#include <iostream>

static std::vector<uint16_t> decode_offsets(BitstreamReader& bitstream, const std::array<uint16_t, 14>& offset_dictionary, uint16_t width, uint16_t height)
{
    std::vector<uint16_t> tile_offsets;
    tile_offsets.resize(width * height * 2, 0);  // Two times the full tile count to account for both background & foreground layers

    uint32_t current_addr = 0;
    while(current_addr < tile_offsets.size())
    {
        uint8_t dictionary_id = bitstream.read_bits(3);
        if(dictionary_id >= 6)
        {
            // If id is 6 or 7, "extend" the dictionary_id by reading a few more bits (allowing for ids up to 13)
            uint8_t extension = ((dictionary_id & 0x01) << 2) | bitstream.read_bits(2);
            dictionary_id = 6 + extension;
        }
        tile_offsets[current_addr] = offset_dictionary[dictionary_id];

        // If next bit is set, copy the same offset value on next row
        if(bitstream.next_bit())
        {
            uint16_t row_addr = current_addr;
            // There is another bit read to determine if the propagation on next rows is:
            //   - in straight line (0)
            //   - diagonally (1)
            bool width_offset = bitstream.next_bit();

            do {
                do {
                    row_addr += width + (width_offset ? 1 : 0);
                    tile_offsets[row_addr] = offset_dictionary[dictionary_id];
                }
                while(bitstream.next_bit()); // While bits are set, keep in the same mode (straight line / diagonal)

                width_offset = !width_offset;
            }
            while(bitstream.next_bit()); // If bit is 0, stop copying this offset
        }

        uint32_t skipped_count = bitstream.read_variable_length_number() + 1;
        current_addr += skipped_count;
    }

    return tile_offsets;
}

static uint8_t count_bits(uint16_t num)
{
    uint8_t exp = 0;
    while(num > 0)
    {
        num >>= 1;
        exp += 1;
    }
    return exp;
}

/**
 * This function read actual tile data using the previously built array of offsets.
 * For any given index, if offset is 0xFFFF, we read data from the ROM. Otherwise,
 * we copy data from X bytes earlier, where X is the offset value.
 *
 * @param bitstream a bit stream used to read the ROM contents bit by bit
 * @param tile_offsets an array of offsets (built using `decode_offsets`)
 * @param original_tile_dictionary a pair of tiles used to decompress data from the ROM
 * @return an array containing the actual indexes of tiles in the map layout blockset
 */
static std::vector<uint16_t> decode_tile_values(BitstreamReader& bitstream,
                                                const std::vector<uint16_t>& tile_offsets,
                                                const std::pair<uint16_t, uint16_t>& original_tile_dictionary)
{
    std::pair<uint16_t, uint16_t> tile_dictionary = original_tile_dictionary;

    std::vector<uint16_t> tile_data;
    tile_data.resize(tile_offsets.size(), 0);

    uint16_t last_nonzero_offset = 0;
    for(size_t current_addr = 0 ; current_addr < tile_offsets.size() ; ++current_addr)
    {
        uint16_t offset = tile_offsets[current_addr];
        if(offset == 0x0000)
            offset = last_nonzero_offset;
        else
            last_nonzero_offset = offset;

        if(offset == 0xFFFF)
        {
            // Offset is 0xFFFF, it means we need to read new data from the ROM
           uint8_t operand = bitstream.read_bits(2);
           if(operand == 0)
           {
               // Operand 0: value is read from the X next bits of data, where X is the bit count of the first
               //            tile in the dictionary pair
               tile_data[current_addr] = bitstream.read_bits(count_bits(tile_dictionary.first));
           }
           else if(operand == 1)
           {
               // Operand 1: value is read from the X next bits of data, where X is the bit count of the amount
               //            the second tile in the dictionary pair increased since the beginning of the algorithm
               uint8_t bit_count = count_bits(tile_dictionary.second - original_tile_dictionary.second);
               uint16_t value = original_tile_dictionary.second + bitstream.read_bits(bit_count);
               tile_data[current_addr] = value;
           }
           else if(operand == 2 || operand == 3)
           {
               // Operand 2 (or 3): use value from first (or second) tile dictionary and increment it
               uint16_t& used_tile = (operand == 2) ? tile_dictionary.first : tile_dictionary.second;
               tile_data[current_addr] = used_tile;
               used_tile += 1;
           }
        }
        else
        {
            // Offset is not 0xFFFF, it means we need to copy previously read data
            tile_data[current_addr] = tile_data[current_addr - offset];
        }
    }

    return tile_data;
}

static std::vector<uint16_t> decode_heightmap(BitstreamReader& bitstream, std::pair<uint8_t, uint8_t> heightmap_size)
{
    std::vector<uint16_t> heightmap_data;
    heightmap_data.resize(heightmap_size.first * heightmap_size.second, 0);

    uint16_t current_offset=0;
    while(current_offset < heightmap_data.size())
    {
        uint16_t rle_value = bitstream.read_bits(16);
        uint8_t rle_count = 0xFF;
        while(rle_count == 0xFF)
        {
            rle_count = bitstream.read_bits(8);
            for(uint8_t i = 0 ; i < rle_count ; ++i)
                heightmap_data[current_offset++] = rle_value;
        }
        heightmap_data[current_offset++] = rle_value;
    }

    return heightmap_data;
}

MapLayout* io::decode_map_layout(const md::ROM& rom, uint32_t addr)
{
    BitstreamReader bitstream(rom.iterator_at(addr));

    uint8_t left = bitstream.read_bits(8);
    uint8_t top = bitstream.read_bits(8);
    uint8_t width = bitstream.read_bits(8) + 1;
    uint8_t height = (bitstream.read_bits(8) + 1) / 2;

    MapLayout* map_layout = new MapLayout(left, top, width, height);

    std::pair<uint16_t, uint16_t> tile_dictionary {};
    tile_dictionary.second = bitstream.read_bits(10);
    tile_dictionary.first = bitstream.read_bits(10);

    std::array<uint16_t, 14> offset_dictionary {
            0xFFFF, // Read new element
            0x0001, // Previous element
            0x0002, // Element before previous element
            static_cast<uint16_t>(width), // Element in previous row
            static_cast<uint16_t>(width * 2), // Element two rows before
            static_cast<uint16_t>(width + 1), // ...
            // Plus 8 dynamic values determined during compression
            0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000
    };
    for(std::size_t i = 6; i < 14; ++i)
        offset_dictionary[i] = bitstream.read_bits(12);

    // Skip a bit that is systematically 0
    bitstream.next_bit();

    std::vector<uint16_t> tile_offsets = decode_offsets(bitstream, offset_dictionary, width, height);
    std::vector<uint16_t> tile_values = decode_tile_values(bitstream, tile_offsets, tile_dictionary);

    auto halfway_iter = tile_values.begin() + (width * height);
    map_layout->foreground_tiles(std::vector<uint16_t>(tile_values.begin(), halfway_iter));
    map_layout->background_tiles(std::vector<uint16_t>(halfway_iter, tile_values.end()));

    bitstream.skip_byte_remainder();

    std::pair<uint8_t, uint8_t> heightmap_size { bitstream.read_bits(8), bitstream.read_bits(8) };
    std::vector<uint16_t> heightmap_data = decode_heightmap(bitstream, heightmap_size);
    map_layout->heightmap(heightmap_data, heightmap_size);

    return map_layout;
}