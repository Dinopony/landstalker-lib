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

    // exports.cpp
    void export_item_sources_as_json(const World& world, const std::string& file_path);
    void export_items_as_json(const World& world, const std::string& file_path);
    void export_entity_types_as_json(const World& world, const std::string& file_path);
    void export_maps_as_json(const World& world, const std::string& file_path);
    void export_map_connections_as_json(const World& world, const std::string& file_path);
    void export_map_palettes_as_json(const World& world, const std::string& file_path);
    void export_teleport_trees_as_json(const World& world, const std::string& file_path);
    void export_game_strings_as_json(const World& world, const std::string& file_path);
    void export_blocksets_as_csv(const World& world, const std::string& blocksets_directory);

    // world_rom_writer.cpp
    void write_world_to_rom(const World& world, md::ROM& rom);
}
