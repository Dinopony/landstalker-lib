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

    [[nodiscard]] std::vector<std::string> to_csv() const
    {
        std::vector<std::string> csv;

        for(const Block& block : _blocks)
        {
            std::string line;
            line = block[0].to_csv() + ","
                    + block[1].to_csv() + ","
                    + block[2].to_csv() + ","
                    + block[3].to_csv();
            csv.push_back(line);
        }

        return csv;
    }
};
