#pragma once

#include "../md_tools.hpp"

class Blockset;
class World;

namespace io {
    // blocksets_decoder.cpp
    Blockset* decode_blockset(const md::ROM& rom, uint32_t addr);

    // world_rom_reader.cpp
    void read_maps(const md::ROM& rom, World& world);
    void read_game_strings(const md::ROM& rom, World& world);
    void read_entity_types(const md::ROM& rom, World& world);
    void read_blocksets(const md::ROM& rom, World& world);
    void read_tilesets(const md::ROM& rom, World& world);

    // world_rom_writer.cpp
    void write_world_to_rom(const World& world, md::ROM& rom);
}
