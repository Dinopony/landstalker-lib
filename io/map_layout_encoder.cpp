#include "io.hpp"
#include "../md_tools.hpp"
#include "../tools/bitstream_writer.hpp"
#include "../tools/byte_array.hpp"
#include "../model/map_layout.hpp"

#include <array>
#include <map>
#include <algorithm>

class UnreachableValueException : public std::exception
{
public:
    uint16_t value;
    explicit UnreachableValueException(uint16_t val) : value(val)
    {}
};

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

static std::vector<uint8_t> assign_best_offsets_from_dictionary(const std::vector<uint16_t>& offset_dictionary,
                                                                const std::vector<uint16_t>& tile_values)
{
    std::vector<uint8_t> offset_dictionary_ids;
    offset_dictionary_ids.resize(tile_values.size(), 0);

    uint16_t map_width = offset_dictionary[3];
    const std::array<int, 3> priorized_lookbacks {
        // Test the previously used offset in priority, since it is less expensive to repeat this offset than anything else
        // Then test offsets that favorize the use of transversal propagation
        1,
        map_width,
        map_width+1
    };

    for(size_t addr=0 ; addr < tile_values.size() ; ++addr)
    {
        for(uint16_t lookback : priorized_lookbacks)
        {
            if (addr < lookback)
                continue;
            uint8_t priorized_offset_id = offset_dictionary_ids[addr - lookback];

            uint16_t offset = offset_dictionary[priorized_offset_id];
            if(offset <= addr && offset > 0x0000 && tile_values[addr - offset] == tile_values[addr])
            {
                offset_dictionary_ids[addr] = priorized_offset_id;
                break;
            }
        }
        if(offset_dictionary_ids[addr])
            continue;

        // Then test all offsets from the dictionary, and evaluate for how many tiles they can be repeated
        std::array<size_t, 14> sequence_length_for_id { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        for(size_t offset_id = 1 ; offset_id < offset_dictionary.size() ; ++offset_id)
        {
            uint16_t offset = offset_dictionary[offset_id];
            if(offset == 0x0000 || offset > addr)
                continue;
            if(tile_values[addr - offset] != tile_values[addr])
                continue;

            sequence_length_for_id[offset_id] += 1;
            for(size_t next_addr = addr + 1 ; next_addr < tile_values.size() ; ++next_addr)
            {
                if(tile_values[next_addr] != tile_values[next_addr - offset])
                    break;
                sequence_length_for_id[offset_id] += 1;
            }
        }

        // Return the best offset ID (the one to provide the longest repeat sequence)
        size_t longest_sequence = 0;
        uint8_t best_offset_id = 0;
        for(size_t offset_id = 1 ; offset_id < offset_dictionary.size() ; ++offset_id)
        {
            if(sequence_length_for_id[offset_id] > longest_sequence)
            {
                longest_sequence = sequence_length_for_id[offset_id];
                best_offset_id = offset_id;
            }
        }

        offset_dictionary_ids[addr] = best_offset_id;
    }

    return offset_dictionary_ids;
}

static std::vector<uint8_t> deduce_offsets(const std::vector<uint16_t>& tile_values,
                                                 std::vector<uint16_t>& offset_dictionary)
{
    // The goal of offsets is to repeat previously read data in the map layout coding.
    // Compared to a data reading which can easily cost more than 10 bits, a good offset
    // usage can use as many bits to cover a large chunk of tiles.
    // We are only allowed to have 13 different offsets in a full map layout file, and 5 of them are fixed
    // (universally useful offsets: -1, -2, -width, -width+1, -width*2).
    // These 5 universal offsets only cost 3 bits to place, whereas the 8 dynamic ones that we are going to
    // add to the dictionary cost 5 bits to place, so are less efficient as a whole.
    // This has a first consequence: we need to place as many relevant universal offsets as we can
    // before attempting to place any dynamic offsets.
    // The goal is then to maximize the coverage of the remaining "holes" by adding new offsets
    // This works into steps: on each step, we give "scores" to offsets which correspond to the amount of new tiles
    // that are being covered if this offset was added to the dictionary.
    // We then add this new offset to the dictionary, assign the newly covered tiles, and perform another step to
    // cover yet some more holes where data has to be read directly.

    // Prepare a map that list all adresses for a given value, for quick lookup
    std::map<uint16_t, std::vector<size_t>> value_addresses;
    for(size_t i=0 ; i<tile_values.size() ; ++i)
        value_addresses[tile_values[i]].emplace_back(i);

    while(offset_dictionary.size() < 14)
    {
        std::vector<uint8_t> offset_dictionary_ids = assign_best_offsets_from_dictionary(offset_dictionary, tile_values);

        // There is still room for new offsets in the dictionary, try to find the best one to add
        //std::map<uint16_t, size_t> offset_scores;
        std::vector<size_t> offset_scores;
        offset_scores.resize(0x10000, 0);

        // Assign scores to offsets
        uint16_t best_offset = 0;
        size_t best_score = 0;
        for(size_t addr=0 ; addr < tile_values.size() ; ++addr)
        {
            // Ignore already assigned values
            if(offset_dictionary_ids[addr] != 0)
                continue;

            uint16_t value = tile_values[addr];
            for(size_t prev_addr : value_addresses.at(value))
            {
                size_t offset = addr - prev_addr;
                if(prev_addr < addr && offset <= 0xFFFE)
                {
                    size_t& score = offset_scores[offset];
                    score += 1;
                    // Keep track of the offset with the best score
                    if(score > best_score)
                    {
                        best_offset = offset;
                        best_score = score;
                    }
                }
            }
        }

        offset_dictionary.emplace_back(best_offset);
    }

    return assign_best_offsets_from_dictionary(offset_dictionary, tile_values);
}

static void encode_tile_values(const std::vector<uint16_t>& tile_values_written_to_rom,
                               const std::pair<uint16_t, uint16_t>& original_tile_dictionary,
                               uint16_t hint_max_t1,
                               BitstreamWriter& bitstream)
{
    std::pair<uint16_t, uint16_t> tile_dictionary = original_tile_dictionary;

    for(uint16_t value : tile_values_written_to_rom)
    {
        // Find the right operand to write the value with a minimal amount of bits
        if(value == tile_dictionary.second && tile_dictionary.second < hint_max_t1)
        {
            // Operand 3 (costs 2 bits)
            bitstream.add_number(0x3, 2);
            tile_dictionary.second += 1;
            continue;
        }

        if(value == tile_dictionary.first)
        {
            // Operand 2 (costs 2 bits)
            bitstream.add_number(0x2, 2);
            tile_dictionary.first += 1;
            continue;
        }

        if(value >= original_tile_dictionary.second)
        {
            // Operand 1 (costs 2+b bits, where b is low)
            uint8_t bit_count = count_bits(tile_dictionary.second - original_tile_dictionary.second);
            uint16_t max_value_with_bit_count = (1 << bit_count) - 1;
            uint16_t value_diff = value - original_tile_dictionary.second;
            if(value_diff <= max_value_with_bit_count)
            {
                bitstream.add_number(0x1, 2);
                bitstream.add_number(value_diff, bit_count);
                continue;
            }
        }

        // Operand 0 (costs 2+b bits, where b is high)
        bitstream.add_number(0x0, 2);
        uint8_t value_bit_count = count_bits(value);
        uint8_t max_bit_count = count_bits(tile_dictionary.first);
        if(value_bit_count > max_bit_count)
            throw UnreachableValueException(value);

        bitstream.add_number(value, max_bit_count);
    }
}

static uint16_t find_longest_chain(const std::vector<uint16_t>& tile_values_written_to_rom,
                                   uint16_t min_starting_value, uint16_t max_starting_value)
{
    std::map<uint16_t, size_t> possible_start_indexes_by_value;
    for(size_t i=0 ; i < tile_values_written_to_rom.size() ; ++i)
    {
        uint16_t value = tile_values_written_to_rom[i];
        if(value < min_starting_value || value >= max_starting_value)
            continue;
        if(possible_start_indexes_by_value.count(value))
            continue;
        possible_start_indexes_by_value[value] = i;
    }

    uint16_t best_chain_length = 0;
    uint16_t best_chain_value = 0;
    for(auto& [value, index] : possible_start_indexes_by_value)
    {
        uint16_t current_chain_value = value;
        for(size_t i=index ; i < tile_values_written_to_rom.size() ; ++i)
        {
            uint16_t value = tile_values_written_to_rom[i];
            if(value == current_chain_value + 1)
                current_chain_value = value;
        }

        uint16_t chain_length = (current_chain_value - value) + 1;
        if(chain_length > best_chain_length)
        {
            best_chain_value = value;
            best_chain_length = chain_length;
        }
    }

    return best_chain_value;
}

static uint16_t check_for_unreachable_values(const std::vector<uint16_t>& tile_values_written_to_rom,
                                             const std::pair<uint16_t, uint16_t>& original_tile_dictionary,
                                             uint16_t hint_max_t1 = 0xFFFF)
{
    try {
        BitstreamWriter dummy_bitstream;
        encode_tile_values(tile_values_written_to_rom, original_tile_dictionary, hint_max_t1, dummy_bitstream);
        return 0;
    }
    catch(UnreachableValueException& ex) {
        return ex.value;
    }
}

static std::pair<uint16_t, uint16_t> deduce_tile_dictionary(const std::vector<uint16_t>& tile_values_written_to_rom,
                                                            uint16_t& hint_max_t1)
{
    // The tile dictionary contains two values :
    //  - T0, which allows to read for values having less or as many bits as its value
    //  - T1, which enables reading values equal or slightly greater than its value
    // This means that with a bad dictionary choice, some high values might become unreachable.
    // Low values can never be unreachable, because they have a bit count lower or equal to T0, but high values
    // can have a bit count higher than T0 (which means that they cannot be reached using T0), and can also be
    // either strictly lower than T1 or much greater than T1, meaning they cannot be reached using T1 either.
    // Having a T0 with the max value bit_count works in every case, but is very inefficient since every read
    // using it will use as many bits.
    // Therefore, we need to minimize T0 bit count while finding a way to reach all the values having a bit count
    // higher than T0's using T1.

    // The method we apply here is the following:
    //  1) Find the maximum value of the whole data set. This will be the hardest value to reach, so the choice for
    //     T1 will largely depend on it.
    //  2) Try to find a "chain" of values for T1 to increase up to max value. We do this by starting from max value
    //     first instance index, and looking backwards to find the starting value for this chain.
    //  3) The starting value for this chain is a very good theoretical candidate for T1.
    //  4) We now find the max value below T1: its bit count will be the bit count for T0, since it needs to be able
    //     to reach that value.
    //          - 4a) If bit count is lower than max value's, all good, keep going
    //          - 4b) If bit count is the same between max value and this one, this means T0 will cover the full dataset
    //                and we need to use another paradigm (see paradigm Z at the end)
    //  5) Now that we know T0's ideal bit count, we look for the biggest chain both starting and ending with a value
    //     having the appropriate bit count.
    //  6) The starting value for this chain is a very good theoretical candidate for T0.
    //  7) We have a good theoretical pair of reference tiles, but we need to check if no values are unreachable.
    //     To do that, we perform a "simulation" of the tiles encoding on a fake Bitstream using the current reference
    //     tiles.
    //          - 7a) If everything goes smooth, good! We have one of the very best combinations possible.
    //          - If there is an unreachable value, we need to fine-tune our approach to try to solve it. This
    //            unreachable value can only be inside T1's sector, so we add a "hint" to tell the algorithm to
    //            stop using the operand increasing T1 once we reach this problematic value, to make it still reachable
    //            at any point. This can result in several ways:
    //                  * 7b) It works: good! Just call the encoding algorithm with this additionnal hint.
    //                  * 7c) It makes a value greater than this hinted value unreachable: this is unsolvable, we need to
    //                    make T0 cover the full bit count range to prevent this problem (see paradigm Z at the end)
    //                  * 7d) A value lower than this hinted value is still unreachable: lower the hint, and retry (even
    //                    if with every retry, there is less and less chances to be able to reach the maximum value)
    // --------------------------
    //  Z) If required, we perform this change of paradigm:
    //      - T0 now uses T1's original value
    //      - T1 now becomes the start of the biggest chain (aside from T0)

    std::pair<uint16_t, uint16_t> tile_dictionary;
    hint_max_t1 = 0xFFFF;

    // Find first instance of max value
    uint16_t max_value = *std::max_element(tile_values_written_to_rom.begin(), tile_values_written_to_rom.end());
    size_t first_max_instance_index = tile_values_written_to_rom.size();
    for(size_t i=0 ; i<tile_values_written_to_rom.size() ; ++i)
    {
        if(tile_values_written_to_rom[i] == max_value)
        {
            first_max_instance_index = i;
            break;
        }
    }

    // Find longest chain to first instance of max
    uint16_t current_chain_start = max_value;
    for(int i = (int)first_max_instance_index-1 ; i >= 0 ; --i)
    {
        if(tile_values_written_to_rom[i] == current_chain_start-1)
            current_chain_start -= 1;
    }

    tile_dictionary.second = current_chain_start;

    // Get max value strictly lower than T1
    uint16_t max_value_lower_than_t1 = 0;
    for(uint16_t value : tile_values_written_to_rom)
        if(value > max_value_lower_than_t1 && value < tile_dictionary.second)
            max_value_lower_than_t1 = value;

    uint8_t max_value_bit_count = count_bits(max_value);
    uint8_t t0_bit_count = count_bits(max_value_lower_than_t1);

    if(t0_bit_count == max_value_bit_count)
    {
        // Paradigm Z: T0 has full bit length and T1 optimizes the longest chain
        tile_dictionary.first = tile_dictionary.second;
        tile_dictionary.second = find_longest_chain(tile_values_written_to_rom, 0, tile_dictionary.second);
        return tile_dictionary;
    }

    // Set T0 to longest chain in its domain
    uint16_t minimal_value_with_t0_bit_count = (1 << (t0_bit_count-1));
    tile_dictionary.first = find_longest_chain(tile_values_written_to_rom, minimal_value_with_t0_bit_count, tile_dictionary.second);

    uint16_t unreachable_value = check_for_unreachable_values(tile_values_written_to_rom, tile_dictionary);
    if(!unreachable_value)
        return tile_dictionary;

    // We add a "hint" to tell the algorithm to stop using the operand increasing T1 once we reach the problematic value
    while(count_bits(unreachable_value) > t0_bit_count)
    {
        uint16_t unreachable_value_2 = check_for_unreachable_values(tile_values_written_to_rom, tile_dictionary,
                                                                    unreachable_value);
        if(!unreachable_value_2)
        {
            hint_max_t1 = unreachable_value;
            return tile_dictionary;
        }

        if(unreachable_value_2 > unreachable_value)
            break;

        unreachable_value = unreachable_value_2;
    }

    // Paradigm Z: T0 has full bit length and T1 optimizes the longest chain
    tile_dictionary.first = tile_dictionary.second;
    tile_dictionary.second = find_longest_chain(tile_values_written_to_rom, 0, tile_dictionary.second);
    return tile_dictionary;
}

static uint16_t check_for_transversal_similitudes(const std::vector<uint8_t>& offset_dictionary_ids,
                                                 size_t addr, uint8_t width, bool diagonally)
{
    size_t cursor = addr;

    for(uint16_t amount = 0 ; true ; ++amount)
    {
        cursor += width;
        if(diagonally)
            cursor += 1;

        if(cursor >= offset_dictionary_ids.size())
            return amount;
        if(offset_dictionary_ids[cursor] != offset_dictionary_ids[addr])
            return amount;
        if(offset_dictionary_ids[cursor-1] == offset_dictionary_ids[addr])
            return amount;
    }
}

static void encode_offsets(const std::vector<uint8_t>& offset_dictionary_ids,
                           uint8_t width,
                           BitstreamWriter& bitstream)
{
    std::vector<bool> already_set_offsets;
    already_set_offsets.resize(offset_dictionary_ids.size(), false);

    bitstream.add_bit(false);

    size_t current_addr = 0;
    while(current_addr < offset_dictionary_ids.size())
    {
        uint8_t dictionary_id = offset_dictionary_ids[current_addr];
        if(dictionary_id < 6)
            bitstream.add_number(dictionary_id, 3);
        else
        {
            // If id is 6 or 7, "extend" the dictionary_id by adding a few more bits (allowing for ids up to 13)
            uint8_t extension = dictionary_id - 6;
            uint8_t extension_msb = (extension & 0x4) >> 2;
            uint8_t first_bits = 6 + extension_msb; // 6 or 7
            bitstream.add_number(first_bits, 3);
            bitstream.add_number(extension & 0x3, 2);
        }

        bool transversal_propagation = false;
        bool diagonal_mode = false;
        if(check_for_transversal_similitudes(offset_dictionary_ids, current_addr, width, false))
        {
            transversal_propagation = true;
        }
        else if(check_for_transversal_similitudes(offset_dictionary_ids, current_addr, width, true))
        {
            transversal_propagation = true;
            diagonal_mode = true;
        }

        bitstream.add_bit(transversal_propagation);
        if(transversal_propagation)
            bitstream.add_bit(diagonal_mode);

        size_t transversal_cursor = current_addr;
        while(transversal_propagation)
        {
            uint16_t count = check_for_transversal_similitudes(offset_dictionary_ids, transversal_cursor, width,
                                                      diagonal_mode);

            size_t movement_unit = width + ((diagonal_mode) ? 1 : 0);
            for(uint16_t i=0 ; i<count ; ++i)
            {
                transversal_cursor += movement_unit;
                already_set_offsets[transversal_cursor] = true;
                for(size_t repeat_cursor = transversal_cursor+1 ; repeat_cursor < offset_dictionary_ids.size() ; ++repeat_cursor)
                {
                    if(offset_dictionary_ids[repeat_cursor] != offset_dictionary_ids[transversal_cursor])
                        break;
                    already_set_offsets[repeat_cursor] = true;
                }

                if(i>0)
                    bitstream.add_bit(true);
            }
            bitstream.add_bit(false);

            diagonal_mode = !diagonal_mode;
            transversal_propagation = check_for_transversal_similitudes(offset_dictionary_ids, transversal_cursor,
                                                                        width, diagonal_mode);
            bitstream.add_bit(transversal_propagation);
        }

        uint32_t skipped_count = 0;

        // Skip all offsets that are the same as the current one
        while((current_addr+1) < offset_dictionary_ids.size())
        {
            uint8_t next_dictionary_id = offset_dictionary_ids[current_addr + 1];
            if(next_dictionary_id != dictionary_id)
                break;
            current_addr += 1;
            skipped_count += 1;
        }

        // Skip all offsets that have already been set, likely due to row propagation
        while((current_addr+1) < offset_dictionary_ids.size())
        {
            if(!already_set_offsets[current_addr + 1])
                break;
            current_addr += 1;
            skipped_count += 1;
        }

        bitstream.add_variable_length_number(skipped_count);
        current_addr += 1;
    }
}



static void encode_heightmap(const std::vector<uint16_t>& heightmap_data, BitstreamWriter& bitstream)
{
    uint16_t current_offset=0;
    while(current_offset < heightmap_data.size())
    {
        uint16_t rle_value = heightmap_data[current_offset];
        bitstream.add_number(rle_value, 16);

        uint16_t rle_count = 1;
        while(current_offset + rle_count < heightmap_data.size() && heightmap_data[current_offset + rle_count] == rle_value)
            rle_count += 1;
        current_offset += rle_count;
        rle_count -= 1;

        while(rle_count >= 0xFF)
        {
            bitstream.add_number(0xFF, 8);
            rle_count -= 0xFF;
        }
        bitstream.add_number(rle_count, 8);
    }
}

ByteArray io::encode_map_layout(MapLayout* map_layout)
{
    // -----------------------------------------------
    //      Setup phase
    // -----------------------------------------------
    std::vector<uint16_t> all_tiles = map_layout->foreground_tiles();
    all_tiles.insert(all_tiles.end(), map_layout->background_tiles().begin(), map_layout->background_tiles().end());

    std::vector<uint16_t> offset_dictionary =  {
            0xFFFF, // Read new element
            0x0001, // Previous element
            0x0002, // Element before previous element
            static_cast<uint16_t>(map_layout->width()), // Element in previous row
            static_cast<uint16_t>(map_layout->width() * 2), // Element two rows before
            static_cast<uint16_t>(map_layout->width() + 1), // ...
    };

    std::vector<uint8_t> offset_dictionary_ids = deduce_offsets(all_tiles, offset_dictionary);
    offset_dictionary.resize(14, 0);

    std::vector<uint16_t> tile_values_written_to_rom;
    tile_values_written_to_rom.reserve(all_tiles.size() / 4);
    for(size_t addr=0 ; addr < all_tiles.size() ; ++addr)
        if(offset_dictionary_ids[addr] == 0) // Offset 0xFFFF ==> data written to ROM
            tile_values_written_to_rom.emplace_back(all_tiles[addr]);

    uint16_t hint_max_t1;
    std::pair<uint16_t, uint16_t> tile_dictionary = deduce_tile_dictionary(tile_values_written_to_rom, hint_max_t1);

    // -----------------------------------------------
    //      Encoding phase
    // -----------------------------------------------
    BitstreamWriter bitstream;
    bitstream.add_number(map_layout->left(), 8);
    bitstream.add_number(map_layout->top(), 8);
    bitstream.add_number(map_layout->width() - 1, 8);
    bitstream.add_number(2*map_layout->height() - 1, 8);

    bitstream.add_number(tile_dictionary.second, 10);
    bitstream.add_number(tile_dictionary.first, 10);

    for(std::size_t i = 6; i < 14; ++i)
        bitstream.add_number(offset_dictionary[i], 12);

    encode_offsets(offset_dictionary_ids, map_layout->width(), bitstream);
    encode_tile_values(tile_values_written_to_rom, tile_dictionary, hint_max_t1, bitstream);

    bitstream.skip_byte_remainder();

    bitstream.add_number(map_layout->heightmap_width(), 8);
    bitstream.add_number(map_layout->heightmap_height(), 8);
    encode_heightmap(map_layout->heightmap(), bitstream);

    ByteArray map_layout_bytes(bitstream.bytes());
    return map_layout_bytes;
}
