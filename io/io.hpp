#pragma once

#include "../md_tools.hpp"

class Blockset;
class World;
class ByteArray;
class HuffmanTree;

namespace io {
    // blocksets_decoder.cpp
    Blockset* decode_blockset(const md::ROM& rom, uint32_t addr);
    // blocksets_encoder.cpp
    ByteArray encode_blockset(Blockset* blockset);

    // textbanks_decoder.cpp
    HuffmanTree* decode_huffman_tree(const md::ROM& rom, uint32_t addr);
    std::vector<std::string> decode_textbanks(const md::ROM& rom, const std::vector<uint32_t>& addrs, const std::vector<HuffmanTree*>& huffman_trees);
    // textbanks_encoder.cpp
    std::vector<HuffmanTree*> build_trees_from_strings(const std::vector<std::string>& strings);
    ByteArray encode_huffman_trees(const std::vector<HuffmanTree*>& huffman_trees);
    std::vector<ByteArray> encode_textbanks(const std::vector<std::string>& strings, const std::vector<HuffmanTree*>& huffman_trees);

    // world_rom_reader.cpp
    void read_maps(const md::ROM& rom, World& world);
    void read_game_strings(const md::ROM& rom, World& world);
    void read_entity_types(const md::ROM& rom, World& world);
    void read_blocksets(const md::ROM& rom, World& world);
    void read_items(const md::ROM& rom, World& world);
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
