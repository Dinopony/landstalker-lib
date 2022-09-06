#pragma once

#include "map_connection.hpp"
#include "../md_tools.hpp"
#include "../tools/flag.hpp"
#include "../tools/json.hpp"
#include "../tools/color_palette.hpp"
#include "map_layout.hpp"

#include <map>
#include <vector>

class SpawnLocation;
class ItemSource;
class Item;
class WorldTeleportTree;
class EntityType;
class Map;
class Blockset;

// TODO: Make const accessors which return const pointers (ensuring there is no edition whatsoever)
class World
{
private:
    std::map<uint8_t, Item*> _items;
    std::vector<std::string> _game_strings;
    std::map<uint8_t, EntityType*> _entity_types;
    std::map<uint16_t, Map*> _maps;
    std::vector<MapConnection> _map_connections;
    std::vector<MapPalette*> _map_palettes;
    std::vector<Item*> _chest_contents;
    std::vector<Flag> _starting_flags;

    uint16_t _spawn_map_id = 0;
    uint8_t _spawn_position_x = 0x15;
    uint8_t _spawn_position_y = 0x15;
    uint8_t _spawn_orientation = 0;
    uint16_t _starting_golds = 0;
    uint8_t _starting_life = 0;

    std::vector<std::vector<Blockset*>> _blockset_groups;
    std::vector<MapLayout*> _map_layouts;

    /// Requires PatchImproveLanternHandling to be handled
    std::vector<uint16_t> _dark_maps;

public:
    World() = default;
    ~World();

    [[nodiscard]] const std::map<uint8_t, Item*>& items() const { return _items; }
    std::map<uint8_t, Item*>& items() { return _items; }
    [[nodiscard]] Item* item(uint8_t id) const { return _items.at(id); }
    [[nodiscard]] Item* item(const std::string& name) const;
    Item* add_item(Item* item);
    Item* add_gold_item(uint8_t worth);
    [[nodiscard]] std::vector<Item*> starting_inventory() const;

    [[nodiscard]] const std::vector<std::string>& game_strings() const { return _game_strings; }
    std::vector<std::string>& game_strings() { return _game_strings; }
    [[nodiscard]] uint16_t first_empty_game_string_id(uint16_t initial_index = 0) const;

    [[nodiscard]] const std::vector<uint16_t>& dark_maps() const { return _dark_maps; }
    void dark_maps(const std::vector<uint16_t>& dark_maps) { _dark_maps = dark_maps; }

    [[nodiscard]] uint16_t spawn_map_id() const { return _spawn_map_id; }
    void spawn_map_id(uint16_t value) { _spawn_map_id = value; }

    [[nodiscard]] uint8_t spawn_position_x() const { return _spawn_position_x; }
    void spawn_position_x(uint8_t value) { _spawn_position_x = value; }

    [[nodiscard]] uint8_t spawn_position_y() const { return _spawn_position_y; }
    void spawn_position_y(uint8_t value) { _spawn_position_y = value; }

    [[nodiscard]] uint8_t spawn_orientation() const { return _spawn_orientation; }
    void spawn_orientation(uint8_t value) { _spawn_orientation = value; }

    [[nodiscard]] const std::vector<Flag>& starting_flags() const { return _starting_flags; }
    std::vector<Flag>& starting_flags() { return _starting_flags; }

    [[nodiscard]] uint16_t starting_golds() const { return _starting_golds; }
    void starting_golds(uint16_t golds) { _starting_golds = golds; }

    [[nodiscard]] uint8_t starting_life() const { return _starting_life; }
    void starting_life(uint8_t life) { _starting_life = life; }

    [[nodiscard]] const std::map<uint8_t, EntityType*>& entity_types() const { return _entity_types; }
    [[nodiscard]] EntityType* entity_type(uint8_t id) const { return _entity_types.at(id); }
    [[nodiscard]] EntityType* entity_type(const std::string& name) const;
    void add_entity_type(EntityType* entity_type);

    [[nodiscard]] const std::map<uint16_t, Map*>& maps() const { return _maps; }
    [[nodiscard]] Map* map(uint16_t map_id) const { return _maps.at(map_id); }
    void add_map(Map* map);

    [[nodiscard]] const std::vector<MapConnection>& map_connections() const { return _map_connections; }
    std::vector<MapConnection>& map_connections() { return _map_connections; }
    MapConnection& map_connection(uint16_t map_id_1, uint16_t map_id_2);
    std::vector<MapConnection*> map_connections(uint16_t map_id);
    std::vector<MapConnection*> map_connections(uint16_t map_id_1, uint16_t map_id_2);
    void swap_map_connections(uint16_t map_id_1, uint16_t map_id_2, uint16_t map_id_3, uint16_t map_id_4);

    [[nodiscard]] const std::vector<MapPalette*>& map_palettes() const { return _map_palettes; }
    [[nodiscard]] MapPalette* map_palette(uint8_t id) { return _map_palettes.at(id); }
    [[nodiscard]] uint8_t map_palette_id(MapPalette* palette) const;
    void add_map_palette(MapPalette* palette);
    void clean_unused_map_palettes();

    [[nodiscard]] const std::vector<Item*>& chest_contents() const { return _chest_contents; }
    [[nodiscard]] std::vector<Item*>& chest_contents() { return _chest_contents; }
    [[nodiscard]] Item* chest_contents(size_t chest_id) const { return _chest_contents.at(chest_id); }
    void chest_contents(size_t chest_id, Item* item) { _chest_contents[chest_id] = item; }

    [[nodiscard]] const std::vector<std::vector<Blockset*>>& blockset_groups() const { return _blockset_groups; }
    [[nodiscard]] std::vector<std::vector<Blockset*>>& blockset_groups() { return _blockset_groups; }
    [[nodiscard]] Blockset* blockset(uint8_t primary_id, uint8_t secondary_id) const { return _blockset_groups[primary_id][secondary_id]; }
    [[nodiscard]] std::pair<uint8_t, uint8_t> blockset_id(Blockset* blockset) const;
    void clean_unused_blocksets();

    [[nodiscard]] const std::vector<MapLayout*>& map_layouts() const { return _map_layouts; }
    void add_map_layout(MapLayout* layout) { _map_layouts.emplace_back(layout); }
    void clean_unused_layouts();
};
