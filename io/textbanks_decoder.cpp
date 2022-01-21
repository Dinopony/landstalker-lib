#include "io.hpp"

#include "../constants/offsets.hpp"
#include "../constants/values.hpp"
#include "../tools/huffman_tree.hpp"

HuffmanTree* io::decode_huffman_tree(const md::ROM& rom, uint32_t addr)
{
    HuffmanTree* tree = new HuffmanTree();
    HuffmanTreeNode* current_node = tree->root_node();
    uint32_t current_symbol_addr = addr - 1;

    BitstreamReader bitstream(rom.iterator_at(addr));

    while(true)
    {
        if(!bitstream.next_bit())
        {
            // Branch node case
            current_node->left_child(new HuffmanTreeNode());
            current_node = current_node->left_child();
        }
        else
        {
            // Leaf node case: read the symbol value BEFORE the initial offset
            current_node->symbol(rom.get_byte(current_symbol_addr));
            current_symbol_addr -= 1;

            // If current node's parent has a right child, it means we completed this branch of the tree
            // and we need to get back at a higher level
            while (current_node->parent() && current_node->parent()->right_child())
            {
                current_node = current_node->parent();
            }

            // If we got back to root node, it means the whole tree is finished
            if (current_node == tree->root_node())
                return tree;

            current_node->parent()->right_child(new HuffmanTreeNode());
            current_node = current_node->parent()->right_child();
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////

static std::string decode_string(BitstreamReader& bitstream, const std::vector<HuffmanTree*>& huffman_trees)
{
    std::string string;
    uint8_t previous_symbol = 0x55;

    while (true)
    {
        HuffmanTreeNode* current_node = huffman_trees[previous_symbol]->root_node();
        while(!current_node->is_leaf())
        {
            if (bitstream.next_bit())
                current_node = current_node->right_child();
            else
                current_node = current_node->left_child();
        }

        uint8_t symbol = current_node->symbol();
        if (symbol == 0x55)
            break;

        string += Symbols::TABLE[symbol];
        previous_symbol = symbol;
    }

    return string;
}

std::vector<std::string> io::decode_textbanks(const md::ROM& rom, const std::vector<uint32_t>& addrs, const std::vector<HuffmanTree*>& huffman_trees)
{
    std::vector<std::string> strings;
    strings.reserve(addrs.size() * STRINGS_PER_TEXTBANK);

    for(uint32_t addr : addrs)
    {
        for(uint32_t i = 0 ; i < STRINGS_PER_TEXTBANK ; ++i)
        {
            uint8_t string_length = rom.get_byte(addr);
            if(string_length == 0)
                break;

            BitstreamReader bitstream(rom.iterator_at(addr + 1));
            std::string string = decode_string(bitstream, huffman_trees);
            strings.emplace_back(string);
            addr += string_length;
        }
    }

    return strings;
}

