#include "world.hpp"

#include "entity_type.hpp"
#include "map.hpp"
#include "item.hpp"
#include "blockset.hpp"

#include "../constants/item_codes.hpp"
#include <set>

World::~World()
{
    for (auto& [key, item] : _items)
        delete item;
    for (auto& [id, entity] : _entity_types)
        delete entity;
    for (MapPalette* palette : _map_palettes)
        delete palette;

    std::set<Blockset*> deleted_blocksets;
    for(const std::vector<Blockset*>& group : _blockset_groups)
    {
        for(Blockset* blockset : group)
        {
            if(!deleted_blocksets.count(blockset))
            {
                deleted_blocksets.insert(blockset);
                delete blockset;
            }
        }
    }
}

Item* World::item(const std::string& name) const
{
    if(name.empty())
        return nullptr;

    for (auto& [key, item] : _items)
        if(item->name() == name)
            return item;

    return nullptr;
}

Item* World::add_item(Item* item)
{
    if(_items.count(item->id()))
        throw LandstalkerException("Cannot add item #" + std::to_string(item->id()) + " since there is already one with the same ID (" + _items[item->id()]->name() + ")");

    _items[item->id()] = item;
    return item;
}

Item* World::add_gold_item(uint8_t worth)
{
    uint8_t first_available_id = ITEM_GOLDS_START;

    // Try to find an item with the same worth
    for(uint8_t i=ITEM_GOLDS_START ; i<ITEM_GOLDS_END ; ++i)
    {
        if(_items.count(i) == 0)
            break;

        first_available_id = i+1;
        if(_items[i]->gold_value() == worth)
            return _items[i];
    }

    // If we consumed all item IDs, don't add it you fool!
    if(first_available_id == ITEM_GOLDS_END)
        return nullptr;

    return this->add_item(new ItemGolds(first_available_id, worth));
}

std::vector<Item*> World::starting_inventory() const
{
    std::vector<Item*> starting_inventory;
    for(auto& [id, item] : _items)
    {
        uint8_t item_starting_quantity = item->starting_quantity();
        for(uint8_t i=0 ; i<item_starting_quantity ; ++i)
            starting_inventory.emplace_back(item);
    }
    return starting_inventory;
}

EntityType* World::entity_type(const std::string& name) const
{
    for(auto& [id, enemy] : _entity_types)
        if(enemy->name() == name)
            return enemy;
    return nullptr;
}

void World::add_map(Map* map)
{
    uint16_t map_id = map->id();
    if(_maps.count(map_id))
        delete _maps[map_id];

    _maps[map_id] = map;
}

MapConnection& World::map_connection(uint16_t map_id_1, uint16_t map_id_2)
{
    for(MapConnection& connection : _map_connections)
        if(connection.check_maps(map_id_1, map_id_2))
            return connection;

    throw LandstalkerException("Could not find a map connection between maps " + std::to_string(map_id_1) + " and " + std::to_string(map_id_2));
}

std::vector<MapConnection*> World::map_connections(uint16_t map_id)
{
    std::vector<MapConnection*> connections;
    for(MapConnection& connection : _map_connections)
        if(connection.map_id_1() == map_id || connection.map_id_2() == map_id)
            connections.emplace_back(&connection);

    return connections;
}

std::vector<MapConnection*> World::map_connections(uint16_t map_id_1, uint16_t map_id_2)
{
    std::vector<MapConnection*> connections;
    for(MapConnection& connection : _map_connections)
    {
        if(connection.map_id_1() == map_id_1 && connection.map_id_2() == map_id_2)
            connections.emplace_back(&connection);
        if(connection.map_id_1() == map_id_2 && connection.map_id_2() == map_id_1)
            connections.emplace_back(&connection);
    }

    return connections;
}

void World::swap_map_connections(uint16_t map_id_1, uint16_t map_id_2, uint16_t map_id_3, uint16_t map_id_4)
{
    MapConnection& conn_1 = map_connection(map_id_1, map_id_2);
    MapConnection& conn_2 = map_connection(map_id_3, map_id_4);

    bool right_order = true;
    if(conn_1.map_id_1() != map_id_1)
        right_order = !right_order;
    if(conn_2.map_id_1() != map_id_3)
        right_order = !right_order;
    
    uint16_t stored_map_id = conn_1.map_id_1();
    uint8_t stored_pos_x = conn_1.pos_x_1();
    uint8_t stored_pos_y = conn_1.pos_y_1();
    uint8_t stored_extra_byte = conn_1.extra_byte_1();

    if(right_order)
    {
        conn_1.map_id_1(conn_2.map_id_1());
        conn_1.pos_x_1(conn_2.pos_x_1()); 
        conn_1.pos_y_1(conn_2.pos_y_1());
        conn_1.extra_byte_1(conn_2.extra_byte_1());

        conn_2.map_id_1(stored_map_id);
        conn_2.pos_x_1(stored_pos_x); 
        conn_2.pos_y_1(stored_pos_y);
        conn_2.extra_byte_1(stored_extra_byte);
    }
    else
    {
        conn_1.map_id_1(conn_2.map_id_2());
        conn_1.pos_x_1(conn_2.pos_x_2()); 
        conn_1.pos_y_1(conn_2.pos_y_2());
        conn_1.extra_byte_1(conn_2.extra_byte_2());

        conn_2.map_id_2(stored_map_id);
        conn_2.pos_x_2(stored_pos_x); 
        conn_2.pos_y_2(stored_pos_y);
        conn_2.extra_byte_2(stored_extra_byte);
    }
}

uint8_t World::map_palette_id(MapPalette* palette) const
{
    uint8_t size = _map_palettes.size() & 0x3F;
    for(uint8_t i=0 ; i<size ; ++i)
        if(_map_palettes[i] == palette)
            return i;

    throw LandstalkerException("Could not find id of MapPalette as it doesn't seem to be in world's map palette list");
}

uint16_t World::first_empty_game_string_id(uint16_t initial_index) const
{
    for(size_t i=initial_index ; i<_game_strings.size() ; ++i)
        if(_game_strings[i].empty())
            return (uint16_t) i;
    return _game_strings.size();
}

void World::clean_unused_map_palettes()
{
   std::set<MapPalette*> used_palettes;
    for(auto& [map_id, map] : _maps)
        used_palettes.insert(map->palette());

    for(auto it = _map_palettes.begin() ; it != _map_palettes.end() ; )
    {
        MapPalette* palette = *it;
        if(!used_palettes.count(palette))
        {
            delete palette;
            it = _map_palettes.erase(it);
        }
        else ++it;
    }
}

void World::add_map_palette(MapPalette* palette)
{
    if(_map_palettes.size() == 0x3F)
        throw LandstalkerException("Cannot add palette because maximum number of palettes (" + std::to_string(0x3F) + ") was reached");
    _map_palettes.emplace_back(palette);
}

std::pair<uint8_t, uint8_t> World::blockset_id(Blockset* blockset) const
{
    std::pair<uint8_t, uint8_t> last_encountered_id (0xFF, 0xFF);
    for(size_t i=0 ; i<_blockset_groups.size() ; ++i)
    {
        for(size_t j=0 ; j<_blockset_groups[i].size() ; ++j)
        {
            Blockset* b = _blockset_groups[i][j];
            if(blockset == b)
                last_encountered_id = std::make_pair(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
        }
    }

    if(last_encountered_id.first == 0xFF)
        throw LandstalkerException("Could not find blockset ID for blockset referenced by a map");

    return last_encountered_id;
}

void World::clean_unused_blocksets()
{
    std::set<Blockset*> used_blocksets;
    for(auto& [id, map] : _maps)
        used_blocksets.insert(map->blockset());

    // Iterate over all secondary blocksets, and remove unused ones
    for(auto& blockset_group : _blockset_groups)
    {
        // Only start from one to exclude primary blocksets (which are never directly referenced)
        // Primary blocksets are removed only if all linked secondary blocksets are removed as well
        for(size_t j=1 ; j<blockset_group.size() ; ++j)
        {
            Blockset* blockset = blockset_group[j];
            if(!used_blocksets.count(blockset))
            {
                blockset_group.erase(blockset_group.begin() + (int)j);
                delete blockset;
                --j;
            }
        }
    }

    // Empty eventual blockset groups where only the primary blockset remains (no secondary blockset in the group)
    for(auto& blockset_group : _blockset_groups)
    {
        if(blockset_group.size() == 1)
        {
            delete blockset_group[0];
            blockset_group.clear();
        }
    }
}

void World::clean_unused_layouts()
{
    std::set<MapLayout*> used_layouts;
    for(auto& [map_id, map] : _maps)
        used_layouts.insert(map->layout());

    for(auto it = _map_layouts.begin() ; it != _map_layouts.end() ; )
    {
        MapLayout* layout = *it;
        if(!used_layouts.count(layout))
        {
            delete layout;
            it = _map_layouts.erase(it);
        }
        else ++it;
    }
}

void World::add_entity_type(EntityType* entity_type)
{
    if(_entity_types.count(entity_type->id()))
    {
        throw LandstalkerException("Trying to add an entity type with ID #" + std::to_string(entity_type->id())
                                   + " whereas there already is one.");
    }

    _entity_types[entity_type->id()] = entity_type;
}