#include "world.hpp"

#include "entity_type.hpp"
#include "map.hpp"
#include "item.hpp"
#include "item_source.hpp"
#include "spawn_location.hpp"
#include "blockset.hpp"

#include "../constants/offsets.hpp"
#include "../io/io.hpp"

#include "../exceptions.hpp"

// Include headers automatically generated from model json files
#include "data/entity_type.json.hxx"
#include "data/item_source.json.hxx"

#include <iostream>
#include <set>

World::World(const md::ROM& rom)
{
    // No requirements
    io::read_items(rom, *this);
    io::read_game_strings(rom, *this);
    io::read_blocksets(rom, *this);

    io::read_entity_types(rom, *this);
    this->load_entity_types();

    // Reading map entities might actually require items
    io::read_maps(rom, *this);

    // Require maps & entities
    this->load_item_sources();
}

World::~World()
{
    for (auto& [key, item] : _items)
        delete item;
    for (ItemSource* source : _item_sources)
        delete source;
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

void World::write_to_rom(md::ROM& rom)
{
    this->clean_unused_palettes();
    this->clean_unused_blocksets();
    io::write_world_to_rom(*this, rom);
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
    _items[item->id()] = item;
    return item;
}

Item* World::add_gold_item(uint8_t worth)
{
    uint8_t highest_item_id = _items.rbegin()->first;

    // Try to find an item with the same worth
    for(uint8_t i=ITEM_GOLDS_START ; i<=highest_item_id ; ++i)
        if(_items[i]->gold_value() == worth)
            return _items[i];

    // If we consumed all item IDs, don't add it you fool!
    if(highest_item_id == 0xFF)
        return nullptr;

    return this->add_item(new ItemGolds(highest_item_id+1, worth));
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

ItemSource* World::item_source(const std::string& name) const
{
    for (ItemSource* source : _item_sources)
        if (source->name() == name)
            return source;

    throw std::out_of_range("No source with given name");
}

std::vector<ItemSource*> World::item_sources_with_item(Item* item)
{
    std::vector<ItemSource*> sources_with_item;

    for (ItemSource* source : _item_sources)
        if (source->item() == item)
            sources_with_item.emplace_back(source);

    return sources_with_item;
}

uint8_t World::starting_life() const
{
    if(_custom_starting_life)
        return _custom_starting_life;
    return _spawn_location.starting_life();
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

void World::clean_unused_palettes()
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

void World::load_item_sources()
{
    Json item_sources_json = Json::parse(ITEM_SOURCES_JSON);
    for(const Json& source_json : item_sources_json)
    {
        _item_sources.emplace_back(ItemSource::from_json(source_json, *this));
    }

#ifdef DEBUG
    std::cout << _item_sources.size() << " item sources loaded." << std::endl;
#endif

    // The following chests are absent from the game on release or modded out of the game for the rando, and their IDs are therefore free:
    // 0x0E (14): Mercator Kitchen (variant?)
    // 0x1E (30): King Nole's Cave spiral staircase (variant with enemies?) ---> 29 is the one used in rando
    // 0x20 (32): Boulder chase hallway (variant with enemies?) ---> 31 is the one used in rando
    // 0x25 (37): Thieves Hideout entrance (variant with water)
    // 0x27 (39): Thieves Hideout entrance (waterless variant)
    // 0x28 (40): Thieves Hideout entrance (waterless variant)
    // 0x33 (51): Thieves Hideout second room (waterless variant)
    // 0x3D (61): Thieves Hideout reward room (Kayla cutscene variant)
    // 0x3E (62): Thieves Hideout reward room (Kayla cutscene variant)
    // 0x3F (63): Thieves Hideout reward room (Kayla cutscene variant)
    // 0x40 (64): Thieves Hideout reward room (Kayla cutscene variant)
    // 0x41 (65): Thieves Hideout reward room (Kayla cutscene variant)
    // 0xBB (187): Crypt (Larson. E room)
    // 0xBC (188): Crypt (Larson. E room)
    // 0xBD (189): Crypt (Larson. E room)
    // 0xBE (190): Crypt (Larson. E room)
    // 0xC3 (195): Map 712 / 0x2C8 ???
}

void World::load_entity_types()
{
    // Apply the randomizer model changes to the model loaded from ROM
    Json entities_json = Json::parse(ENTITIES_JSON);
    for(auto& [id_string, entity_json] : entities_json.items())
    {
        uint8_t id = std::stoi(id_string);
        if(!_entity_types.count(id))
            _entity_types[id] = EntityType::from_json(id, entity_json, *this);
        else
            _entity_types[id]->apply_json(entity_json, *this);
    }

#ifdef DEBUG
    std::cout << _entity_types.size()  << " entities loaded." << std::endl;
#endif
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