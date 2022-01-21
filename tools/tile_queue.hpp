#pragma once

#include <deque>

class TileQueue {
private:
    std::deque<uint16_t> _tiles;

public:
    explicit TileQueue(size_t size) { _tiles.resize(size, 0); }

    void move_to_front(int index)
    {
        std::rotate(_tiles.begin(), _tiles.begin() + index, _tiles.begin() + index + 1);
    }

    void push_front(uint16_t value)
    {
        _tiles.push_front(value);
        _tiles.pop_back();
    }

    int find(uint16_t value)
    {
        auto it = std::find(_tiles.begin(), _tiles.end(), value);
        if(it == _tiles.end())
            return -1;
        return (int)(it-_tiles.begin());
    }

    [[nodiscard]] uint16_t front() const { return _tiles.front(); }
};
