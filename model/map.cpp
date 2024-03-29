#include "map.hpp"
#include "entity.hpp"
#include "../tools/color_palette.hpp"
#include "world.hpp"

#include "../constants/offsets.hpp"
#include "../exceptions.hpp"

Map::Map(uint16_t map_id) :
    _id                 (map_id)
{}

Map::Map(const Map& map) :
    _id                         (map._id),
    _layout                     (map._layout),
    _blockset                   (map._blockset),
    _palette                    (map._palette),
    _room_height                (map._room_height),
    _background_music           (map._background_music),
    _unknown_param_1            (map._unknown_param_1),
    _unknown_param_2            (map._unknown_param_2),
    _base_chest_id              (map._base_chest_id),
    _fall_destination           (map._fall_destination),
    _climb_destination          (map._climb_destination),
    _parent_map                 (map._parent_map),
    _variants                   (map._variants),
    _global_entity_mask_flags   (map._global_entity_mask_flags)
{
    for(Entity* entity : map._entities)
        this->add_entity(new Entity(*entity));
}

Map::~Map()
{
    for(Entity* entity : _entities)
        delete entity;
}

void Map::clear()
{
    _base_chest_id = 0x00;
    _fall_destination = 0xFFFF;
    _climb_destination = 0xFFFF;
    _layout = nullptr;
    _blockset = nullptr;
    _palette = nullptr;
    _entities.clear();
    _variants.clear();
    _parent_map = nullptr;
    _global_entity_mask_flags.clear();
    _speaker_ids.clear();
    _map_setup_addr = 0xFFFFFFFF;
    _map_update_addr = 0xFFFFFFFF;
}

////////////////////////////////////////////////////////////////

Entity* Map::add_entity(Entity* entity) 
{
    entity->map(this);
    _entities.emplace_back(entity);
    return entity;
}

void Map::insert_entity(uint8_t entity_id, Entity* entity) 
{
    entity->map(this);
    _entities.insert(_entities.begin() + entity_id, entity);

    // Shift entity indexes in clear flags
    for(GlobalEntityMaskFlag& global_mask_flag : _global_entity_mask_flags)
    {
        if(entity_id <= global_mask_flag.first_entity_id)
            global_mask_flag.first_entity_id += 1;
    }

    for(GlobalEntityMaskFlag& global_mask_flag : _key_door_mask_flags)
    {
        if(entity_id <= global_mask_flag.first_entity_id)
            global_mask_flag.first_entity_id += 1;
    }
}

uint8_t Map::entity_id(const Entity* entity) const
{
    for(uint8_t id=0 ; id < _entities.size() ; ++id)
        if(_entities[id] == entity)
            return id;
    throw LandstalkerException("Prompting entity ID of entity not inside map.");
}

void Map::remove_entity(uint8_t entity_id, bool delete_pointer) 
{ 
    Entity* erased_entity = _entities.at(entity_id);
    erased_entity->map(nullptr);
    _entities.erase(_entities.begin() + entity_id);

    // If any other entity were using tiles from this entity, clear that
    for(Entity* entity : _entities)
    {
        if(entity->entity_to_use_tiles_from() == erased_entity)
            entity->entity_to_use_tiles_from(nullptr);
    }

    // Shift entity indexes in clear flags
    for(uint8_t i=0 ; i<_global_entity_mask_flags.size() ; ++i)
    {
        GlobalEntityMaskFlag& global_mask_flag = _global_entity_mask_flags[i];
        if(global_mask_flag.first_entity_id >= _entities.size())
        {
            _global_entity_mask_flags.erase(_global_entity_mask_flags.begin() + i);
            --i;
        }
        else if(global_mask_flag.first_entity_id > entity_id)
            global_mask_flag.first_entity_id--;
    }

    if(delete_pointer)
        delete erased_entity;
}

void Map::move_entity(uint8_t entity_id, uint8_t entity_new_id)
{
    Entity* entity_to_move = _entities.at(entity_id); 
    this->remove_entity(entity_id, false);
    this->insert_entity(entity_new_id, entity_to_move);
} 

void Map::clear_entities()
{
    _entities.clear();
    _global_entity_mask_flags.clear();
    _key_door_mask_flags.clear();
    if(_variants.empty())
        _speaker_ids.clear();
}

void Map::convert_global_masks_into_individual()
{
    for(const GlobalEntityMaskFlag& flag : _global_entity_mask_flags)
    {
        for(size_t i=flag.first_entity_id ; i<_entities.size() ; ++i)
            _entities[i]->mask_flags().emplace_back(EntityMaskFlag(false, flag.byte, flag.bit));
    }

    for(const GlobalEntityMaskFlag& flag : _key_door_mask_flags)
    {
        for(size_t i=flag.first_entity_id ; i<_entities.size() ; ++i)
            _entities[i]->mask_flags().emplace_back(EntityMaskFlag(false, flag.byte, flag.bit));
    }

    _global_entity_mask_flags.clear();
    _key_door_mask_flags.clear();
}

////////////////////////////////////////////////////////////////

void Map::add_variant(Map* variant_map, uint8_t flag_byte, uint8_t flag_bit)
{
    _variants[variant_map] = Flag(flag_byte, flag_bit);
    variant_map->_parent_map = this;
}

std::set<Map*> Map::all_recursive_variants() const
{
    std::set<Map*> previous_variants;
    std::set<Map*> current_variants;
    for(auto& [variant, _] : _variants)
        current_variants.insert(variant);

    do {
        previous_variants = current_variants;
        for(Map* map : previous_variants)
        {
            for(auto& [variant, _] : map->_variants)
                current_variants.insert(variant);
        }
    } while(current_variants.size() > previous_variants.size());

    return current_variants;
}

////////////////////////////////////////////////////////////////

Json Map::to_json(const World& world) const
{
    Json json;

    if(_blockset)
        json["blocksetId"] = world.blockset_id(_blockset);
    if(_palette)
        json["paletteId"] = world.map_palette_id(_palette);

    json["roomHeight"] = _room_height;
    json["backgroundMusic"] = _background_music;
    json["baseChestId"] = _base_chest_id;

    if(_fall_destination != 0xFFFF)
        json["fallDestination"] = _fall_destination;
    if(_climb_destination != 0xFFFF)
        json["climbDestination"] = _climb_destination;

    json["visitedFlag"] = _visited_flag.to_json();

    if(!_variants.empty())
    {
        json["variants"] = Json::array();
        for(auto& [map, flag] : _variants)
        {
            json["variants"].push_back({
                { "mapVariantId", map->id() },
                { "flagByte", flag.byte },
                { "flagBit", flag.bit }
            });
        }
    }

    if(!_global_entity_mask_flags.empty())
    {
        json["globalEntityMaskFlags"] = Json::array();
        for(const GlobalEntityMaskFlag& mask_flag : _global_entity_mask_flags)
            json["globalEntityMaskFlags"].emplace_back(mask_flag.to_json());
    }

    if(!_key_door_mask_flags.empty())
    {
        json["keyDoorMaskFlags"] = Json::array();
        for(const GlobalEntityMaskFlag& mask_flag : _key_door_mask_flags)
            json["keyDoorMaskFlags"].emplace_back(mask_flag.to_json());
    }

    if(!_entities.empty())
    {
        json["entities"] = Json::array();
        uint8_t chest_id = _base_chest_id;
        for(Entity* entity : _entities)
        {
            Json entity_json = entity->to_json(world);
            if(entity_json.at("entityType") == "chest")
                entity_json["chestId"] = chest_id++;
            json["entities"].emplace_back(entity_json);
        }
    }
    
    if(!_speaker_ids.empty())
        json["speakers"] = _speaker_ids;

    json["unknownParam1"] = _unknown_param_1;
    json["unknownParam2"] = _unknown_param_2;

    return json;
}
