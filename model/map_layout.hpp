#pragma once

#include <vector>
#include <array>
#include <algorithm>
#include <set>

class MapLayout
{
private:
    std::pair<uint8_t,uint8_t> _offset;
    std::pair<uint8_t,uint8_t> _size;
    std::pair<uint8_t,uint8_t> _heightmap_size;

    std::vector<uint16_t> _foreground_tiles;
    std::vector<uint16_t> _background_tiles;
    std::vector<uint16_t> _heightmap;

public:
    MapLayout(uint8_t left, uint8_t top, uint8_t width, uint8_t height) :
            _offset(left, top),
            _size(width, height),
            _heightmap_size(0,0)
    {}

    [[nodiscard]] uint8_t left() const { return _offset.first; }
    [[nodiscard]] uint8_t top() const { return _offset.second; }
    [[nodiscard]] uint8_t width() const { return _size.first; }
    [[nodiscard]] uint8_t height() const { return _size.second; }

    [[nodiscard]] const std::vector<uint16_t>& foreground_tiles() const { return _foreground_tiles; }
    void foreground_tiles(const std::vector<uint16_t>& foreground_tiles) { _foreground_tiles = foreground_tiles; }

    [[nodiscard]] const std::vector<uint16_t>& background_tiles() const { return _background_tiles; }
    void background_tiles(const std::vector<uint16_t>& background_tiles) { _background_tiles = background_tiles; }

    [[nodiscard]] uint8_t heightmap_width() const { return _heightmap_size.first; }
    [[nodiscard]] uint8_t heightmap_height() const { return _heightmap_size.second; }

    [[nodiscard]] const std::vector<uint16_t>& heightmap() const { return _heightmap; }
    void heightmap(const std::vector<uint16_t>& heightmap, std::pair<uint8_t, uint8_t> heightmap_size)
    {
        _heightmap_size = heightmap_size;
        _heightmap = heightmap;
    }
};
