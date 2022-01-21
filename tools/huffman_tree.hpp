#pragma once

#include <cstdint>
#include <algorithm>

#include "huffman_tree_node.hpp"
#include "../constants/symbols.hpp"
#include "byte_array.hpp"
#include "bitstream_reader.hpp"
#include "bitstream_writer.hpp"

class HuffmanTree
{
private:
    HuffmanTreeNode* _root_node;
    std::map<uint8_t, std::vector<bool>> _encodingTable;

public:
    // Constructor to build a Huffman tree from ROM data
    HuffmanTree() : _root_node(new HuffmanTreeNode())
    {}

    // Constructor to build a Huffman tree from a list of symbol counts
    explicit HuffmanTree(const std::vector<SymbolCount>& successor_counts)
    {
        _root_node = new HuffmanTreeNode();
        this->build_nodes_from_array(_root_node, successor_counts);
        this->build_encoding_table(_root_node);
    }

    ~HuffmanTree()
    {
        delete _root_node;
    }

    [[nodiscard]] HuffmanTreeNode* root_node() const { return _root_node; }

    void build_nodes_from_array(HuffmanTreeNode* fromNode, const std::vector<SymbolCount>& successor_counts)
    {
        if (successor_counts.size() < 2)
        {
            // Leaf node
            fromNode->symbol(successor_counts[0].symbol());
            return;
        }

        // Branch node: find which index is the median in terms of commonness, to then be able to
        // split evenly between the left and right branches
        uint32_t half_weight_sum = 0;
        for (const SymbolCount& sc : successor_counts)
            half_weight_sum += sc.count();
        half_weight_sum /= 2;

        uint32_t current_weight_sum = 0;
        auto it = successor_counts.begin();
        while(current_weight_sum < half_weight_sum && it != successor_counts.end())
        {
            current_weight_sum += it->count();
            ++it;
        }

        std::vector<SymbolCount> leftSide(successor_counts.begin(), it);
        std::vector<SymbolCount> rightSide(it, successor_counts.end());

        fromNode->left_child(new HuffmanTreeNode());
        fromNode->right_child(new HuffmanTreeNode());
        this->build_nodes_from_array(fromNode->left_child(), leftSide);
        this->build_nodes_from_array(fromNode->right_child(), rightSide);
    }

    [[nodiscard]] const std::vector<bool>& encode(uint8_t symbol) const
    {
        return _encodingTable.at(symbol);
    }

    void bytes(std::vector<uint8_t>& value_bytes, std::vector<uint8_t>& structure_bytes)
    {
        BitstreamWriter structure_bitstream;

        HuffmanTreeNode* currentNode = _root_node;
        while (true)
        {
            if (currentNode->is_leaf())
            {
                structure_bitstream.add_bit(true);
                value_bytes.emplace_back(currentNode->symbol());

                // If we are our parent's right child, it means we completed this branch of the tree
                // and we need to get back at a higher level
                while (currentNode->parent() && currentNode->parent()->right_child() == currentNode)
                    currentNode = currentNode->parent();

                // If we got back all the way to root node, it means we explored the full tree
                if (currentNode == _root_node)
                    break;

                currentNode = currentNode->parent()->right_child();
            } 
            else
            {
                structure_bitstream.add_bit(false);
                currentNode = currentNode->left_child();
            }
        }

        std::reverse(value_bytes.begin(), value_bytes.end());
        structure_bytes = structure_bitstream.bytes();
    }

private:
    void build_encoding_table(HuffmanTreeNode* fromNode, const std::vector<bool>& bits = {})
    {
        if (fromNode->is_leaf())
        {
            _encodingTable[fromNode->symbol()] = bits;
        }
        else
        {
            std::vector<bool> bits_plus_zero = bits;
            bits_plus_zero.emplace_back(false);
            this->build_encoding_table(fromNode->left_child(), bits_plus_zero);

            std::vector<bool> bits_plus_one = bits;
            bits_plus_one.emplace_back(true);
            this->build_encoding_table(fromNode->right_child(), bits_plus_one);
        }
    }
};
