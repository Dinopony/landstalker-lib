#pragma once

#include <vector>

class TileQueue {
private:
    std::vector<uint16_t> _tiles;

public:
    explicit TileQueue(size_t size) { _tiles.resize(size, 0); }

    void move_to_front(size_t index)
    {
        uint16_t value = _tiles[index];
        _tiles.erase(_tiles.begin() + static_cast<int32_t>(index));
        _tiles.insert(_tiles.begin(), value);
    }

    void push_front(uint16_t value)
    {
        _tiles.insert(_tiles.begin(), value);
        _tiles.erase(_tiles.begin() + static_cast<int32_t>(_tiles.size() - 1));
    }

    [[nodiscard]] uint16_t front() const { return _tiles.at(0); }
};
