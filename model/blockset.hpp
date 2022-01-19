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

    [[nodiscard]] Json to_json() const
    {
        Json json = Json::array();

        for(const Block& block : _blocks)
        {
            Json block_json = Json::array();
            block_json.push_back(block[0].to_json());
            block_json.push_back(block[1].to_json());
            block_json.push_back(block[2].to_json());
            block_json.push_back(block[3].to_json());
            json.push_back(block_json);
        }

        return json;
    }
};
