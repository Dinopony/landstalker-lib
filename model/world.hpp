#pragma once

#include "spawn_location.hpp"
#include "map_connection.hpp"
#include "../constants/values.hpp"
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

class World
{
private:
    std::map<uint8_t, Item*> _items;
    std::vector<ItemSource*> _item_sources;
    std::vector<std::string> _game_strings;
    std::map<uint8_t, EntityType*> _entity_types;
    std::map<uint16_t, Map*> _maps;
    std::vector<MapConnection> _map_connections;
    std::vector<MapPalette*> _map_palettes;
    std::vector<EntityType*> _fahl_enemies;
    SpawnLocation _spawn_location;
    std::vector<Flag> _starting_flags;
    uint16_t _starting_golds = 0;
    uint8_t _custom_starting_life = 0;

    std::vector<std::vector<Blockset*>> _blockset_groups;
    std::vector<MapLayout*> _map_layouts;

    /// Requires PatchImproveLanternHandling to be handled
    std::vector<uint16_t> _dark_maps;

public:
    explicit World(const md::ROM& rom);
    ~World();

    void write_to_rom(md::ROM& rom);

    [[nodiscard]] const std::map<uint8_t, Item*>& items() const { return _items; }
    std::map<uint8_t, Item*>& items() { return _items; }
    [[nodiscard]] Item* item(uint8_t id) const { return _items.at(id); }
    [[nodiscard]] Item* item(const std::string& name) const;
    Item* add_item(Item* item);
    Item* add_gold_item(uint8_t worth);
    [[nodiscard]] std::vector<Item*> starting_inventory() const;

    [[nodiscard]] const std::vector<ItemSource*>& item_sources() const { return _item_sources; }
    [[nodiscard]] std::vector<ItemSource*>& item_sources() { return _item_sources; }
    [[nodiscard]] ItemSource* item_source(const std::string& name) const;
    [[nodiscard]] std::vector<ItemSource*> item_sources_with_item(Item* item);

    [[nodiscard]] const std::vector<std::string>& game_strings() const { return _game_strings; }
    std::vector<std::string>& game_strings() { return _game_strings; }
    [[nodiscard]] uint16_t first_empty_game_string_id(uint16_t initial_index = 0) const;

    [[nodiscard]] const std::vector<uint16_t>& dark_maps() const { return _dark_maps; }
    void dark_maps(const std::vector<uint16_t>& dark_maps) { _dark_maps = dark_maps; }

    [[nodiscard]] virtual const SpawnLocation& spawn_location() const { return _spawn_location; }
    virtual void spawn_location(const SpawnLocation& spawn) { _spawn_location = spawn; }

    [[nodiscard]] const std::vector<Flag>& starting_flags() const { return _starting_flags; }
    std::vector<Flag>& starting_flags() { return _starting_flags; }

    [[nodiscard]] uint16_t starting_golds() const { return _starting_golds; }
    void starting_golds(uint16_t golds) { _starting_golds = golds; }

    [[nodiscard]] uint8_t starting_life() const;
    [[nodiscard]] uint8_t custom_starting_life() const { return _custom_starting_life; }
    void custom_starting_life(uint8_t life) { _custom_starting_life = life; }

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

    [[nodiscard]] const std::vector<EntityType*>& fahl_enemies() const { return _fahl_enemies; }
    void add_fahl_enemy(EntityType* enemy) { _fahl_enemies.emplace_back(enemy); }

    [[nodiscard]] const std::vector<std::vector<Blockset*>>& blockset_groups() const { return _blockset_groups; }
    [[nodiscard]] std::vector<std::vector<Blockset*>>& blockset_groups() { return _blockset_groups; }
    [[nodiscard]] Blockset* blockset(uint8_t primary_id, uint8_t secondary_id) const { return _blockset_groups[primary_id][secondary_id]; }
    [[nodiscard]] std::pair<uint8_t, uint8_t> blockset_id(Blockset* blockset) const;
    void clean_unused_blocksets();

    [[nodiscard]] const std::vector<MapLayout*>& map_layouts() const { return _map_layouts; }
    void add_map_layout(MapLayout* layout) { _map_layouts.emplace_back(layout); }
    void clean_unused_layouts();

private:
    void load_item_sources();
    void load_entity_types();

    void clean_unused_palettes();
};
