#include "io.hpp"

#include "../tools/bitstream_writer.hpp"
#include "../tools/byte_array.hpp"
#include "../constants/symbols.hpp"
#include "../tools/huffman_tree.hpp"

#include <iostream>
#include <algorithm>
#include <array>

static uint8_t char_to_symbol(char c)
{
    for (uint8_t symbol_id = 0; symbol_id < SYMBOL_COUNT; symbol_id++)
        if(c == Symbols::TABLE[symbol_id])
            return symbol_id;

    return 0xFF;
}

static std::vector<uint8_t> string_to_symbols(const std::string& string)
{
    std::vector<uint8_t> string_as_symbols;
    string_as_symbols.reserve(string.size() + 1);

    for (char c : string)
    {
        uint8_t symbol = char_to_symbol(c);
        if (symbol != 0xFF)
            string_as_symbols.emplace_back(symbol);
        else
            std::cerr << "Current character '" << c << "' is not recognized as the beginning of a known symbol, and will be ignored.\n";
    }

    string_as_symbols.emplace_back(0x55);
    return string_as_symbols;
}

std::vector<HuffmanTree*> io::build_trees_from_strings(const std::vector<std::string>& strings)
{
    // Step 1: Convert strings into LS character table symbols, and count successors for each symbol
    std::array<std::array<uint32_t, SYMBOL_COUNT>, SYMBOL_COUNT> symbol_counts {};
    for (const std::string& string : strings)
    {
        std::vector<uint8_t> string_as_symbols = string_to_symbols(string);

        uint8_t previous_symbol = 0x55;
        for (uint8_t symbol : string_as_symbols)
        {
            symbol_counts[previous_symbol][symbol]++;
            previous_symbol = symbol;
        }
    }

    // Step 2: For each symbol, sort counts by descending order to have the most common symbol chains first
    std::array<std::vector<SymbolCount>, SYMBOL_COUNT> successor_counts_per_symbol;
    for (uint8_t i=0 ; i < SYMBOL_COUNT ; ++i)
    {
        std::vector<SymbolCount>& successors_count = successor_counts_per_symbol[i];
        for (uint8_t j = 0; j < SYMBOL_COUNT; ++j)
            if(symbol_counts[i][j])
                successors_count.emplace_back(SymbolCount(j, symbol_counts[i][j]));

        std::sort(successors_count.begin(), successors_count.end());
    }

    // Step 3: Build Huffman trees using those counts
    std::vector<HuffmanTree*> trees;
    for (const std::vector<SymbolCount>& successor_counts : successor_counts_per_symbol)
    {
        if(!successor_counts.empty())
            trees.emplace_back(new HuffmanTree(successor_counts));
        else
            trees.emplace_back(nullptr);
    }
    return trees;
}

///////////////////////////////////////////////////////////////////////////////

ByteArray io::encode_huffman_trees(const std::vector<HuffmanTree*>& huffman_trees)
{
    ByteArray tree_offsets;
    ByteArray tree_data_bytes;

    for (uint8_t i=0 ; i<SYMBOL_COUNT ; ++i)
    {
        HuffmanTree* tree_for_symbol = huffman_trees[i];
        if (tree_for_symbol)
        {
            std::vector<uint8_t> value_bytes, structure_bytes;
            tree_for_symbol->bytes(value_bytes, structure_bytes);

            tree_data_bytes.add_bytes(value_bytes);
            tree_offsets.add_word((uint16_t)tree_data_bytes.size());
            tree_data_bytes.add_bytes(structure_bytes);
        }
        else
        {
            tree_offsets.add_word(0xFFFF);
        }
    }

    tree_offsets.add_bytes(tree_data_bytes);
    return tree_offsets;
}

///////////////////////////////////////////////////////////////////////////////

std::vector<ByteArray> io::encode_textbanks(const std::vector<std::string>& strings, const std::vector<HuffmanTree*>& huffman_trees)
{
    std::vector<ByteArray> textbanks;
    textbanks.resize((size_t)std::ceil((double)strings.size() / 256.0));

    for (size_t i=0 ; i<strings.size() ; ++i)
    {
        const std::string& string = strings[i];
        if(string.size() > 0xFE)
            throw LandstalkerException("String #" + std::to_string(i) + "is too long to be integrated in textbanks ('" + string + "')");

        ByteArray& current_textbank = textbanks[i / 256];

        std::vector<uint8_t> string_as_symbols = string_to_symbols(string);

        uint8_t previous_symbol = 0x55;
        BitstreamWriter bitstream;
        for (uint8_t symbol : string_as_symbols)
        {
            bitstream.add_bits(huffman_trees[previous_symbol]->encode(symbol));
            previous_symbol = symbol;
        }

        const std::vector<uint8_t>& encoded_bytes = bitstream.bytes();
        current_textbank.add_byte(encoded_bytes.size() + 1);
        current_textbank.add_bytes(encoded_bytes);
    }

    return textbanks;
}
