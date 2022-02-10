#pragma once

#include <vector>
#include <array>
#include "tile.hpp"
#include "../tools/json.hpp"

class Blockset {
public:
    using Block = std::array<Tile, 4>;

private:
    std::vector<Blockset::Block> _blocks;

public:
    explicit Blockset(std::vector<Blockset::Block> blocks) :
        _blocks(std::move(blocks))
    {}

    [[nodiscard]] const std::vector<Blockset::Block>& blocks() const { return _blocks; }
};
