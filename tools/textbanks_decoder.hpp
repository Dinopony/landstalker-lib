#pragma once

#include <string>
#include <vector>
#include "../md_tools.hpp"
#include "huffman/tree.hpp"

class TextbanksDecoder
{
private:
    std::vector<std::string> _strings;
    std::vector<HuffmanTree*> _trees;

public:
    explicit TextbanksDecoder(const md::ROM& rom);

    [[nodiscard]] const std::vector<std::string>& strings() const { return _strings; };
    [[nodiscard]] const std::vector<HuffmanTree*>& trees() const { return _trees; };

private:
    void parse_huffman_trees(const md::ROM& rom);
    void parse_textbanks(const md::ROM& rom);
    std::string parse_string(const std::vector<uint8_t>& data, uint32_t offset);
    static uint8_t parse_next_symbol(HuffmanTree* huffman_tree, const std::vector<uint8_t>& data, uint32_t offset, uint32_t& string_bit_index);
};
