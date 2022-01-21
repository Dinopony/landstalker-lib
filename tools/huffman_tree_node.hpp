#pragma once

#include <cstdint>

class HuffmanTreeNode
{
private:
    HuffmanTreeNode* _leftChild = nullptr;
    HuffmanTreeNode* _rightChild = nullptr;
    HuffmanTreeNode* _parent = nullptr;
    uint8_t _symbol = 0x00;

public:
    HuffmanTreeNode() = default;
    ~HuffmanTreeNode()
    {
        delete _leftChild;
        delete _rightChild;
    }

    void symbol(uint8_t symbol) { _symbol = symbol; }
    [[nodiscard]] const uint8_t& symbol() const { return _symbol; }

    [[nodiscard]] HuffmanTreeNode* left_child() const { return _leftChild; }
    void left_child(HuffmanTreeNode* node)
    { 
        _leftChild = node;
        node->_parent = this; 
    }

    [[nodiscard]] HuffmanTreeNode* right_child() const { return _rightChild; }
    void right_child(HuffmanTreeNode* node)
    {
        _rightChild = node; 
        node->_parent = this; 
    }

    [[nodiscard]] HuffmanTreeNode* parent() const { return _parent; }
    [[nodiscard]] bool is_leaf() const { return !_leftChild && !_rightChild; }
};