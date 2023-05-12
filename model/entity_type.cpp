#include "entity_type.hpp"
#include "world.hpp"

EntityType* EntityType::from_json(uint8_t id, const Json& json, const World& world)
{
    const std::string& name = json.at("name");
    const std::string& type = json.at("type");

    EntityType* entity_type = nullptr;
    if(type == "entity")
    {
        entity_type = new EntityType(id, name);
    }
    else if(type == "enemy")
    {
        uint8_t health = json.value("health", 0);
        uint8_t attack = json.value("attack", 0);
        uint8_t defence = json.value("defence", 99);
        uint8_t dropped_golds = json.value("droppedGolds", 0);
        
        std::string dropped_item_name = json.value("droppedItem", "");
        Item* dropped_item = nullptr;
        if(!dropped_item_name.empty())
            dropped_item = world.item(dropped_item_name);

        uint16_t drop_probability = json.value("dropProbability", 0);

        entity_type = new EnemyType(id, name, health, attack, defence,
                                    dropped_golds, dropped_item, drop_probability);
    }
    else
    {
        return nullptr;
    }

    if(json.contains("paletteLow"))
    {
        EntityLowPalette& palette = entity_type->low_palette();
        for(size_t i=0 ; i<palette.size() ; ++i)
            palette[i] = Color::from_json(json.at("paletteLow")[i]);
    }
    if(json.contains("paletteHigh"))
    {
        EntityHighPalette& palette = entity_type->high_palette();
        for(size_t i=0 ; i<palette.size() ; ++i)
            palette[i] = Color::from_json(json.at("paletteHigh")[i]);
    }

    return entity_type;
}

EnemyType::EnemyType(uint8_t id, const std::string& name,
                     uint8_t health, uint8_t attack, uint8_t defence,
                     uint8_t dropped_golds, Item* dropped_item, uint16_t drop_probability) :
    EntityType          (id, name),
    _health             (health),
    _attack             (attack),
    _defence            (defence),
    _dropped_golds      (dropped_golds),
    _dropped_item       (dropped_item),
    _drop_probability   (drop_probability),
    _unkillable         (false)
{}

void EnemyType::apply_health_factor(double factor)
{
    if(_health < 255 && _health >= 1)
    {
        double factored_health = std::round(static_cast<uint16_t>(_health) * factor);
        if (factored_health > 254)
            factored_health = 254;
        if (factored_health < 1)
            factored_health = 1;
        _health = static_cast<uint8_t>(factored_health);
    }
}

void EnemyType::apply_armor_factor(double factor)
{
    if(_defence < 99 && _defence >= 1)
    {
        double factored_defence = std::round(static_cast<uint16_t>(_defence) * factor);
        if (factored_defence > 98)
            factored_defence = 98;
        if (factored_defence < 1)
            factored_defence = 1;
        _defence = static_cast<uint8_t>(factored_defence);
    }
}

void EnemyType::apply_damage_factor(double factor)
{
    if(_attack < 127 && _attack >= 1)
    {
        double factored_attack = std::round(static_cast<uint16_t>(_attack) * factor);
        if (factored_attack > 126)
            factored_attack = 126;
        if (factored_attack < 1)
            factored_attack = 1;
        _attack = static_cast<uint8_t>(factored_attack);
    }
}

void EnemyType::apply_golds_factor(double factor)
{
    double factored_golds = std::round(static_cast<uint16_t>(_dropped_golds) * factor);
    if (factored_golds > 255)
        factored_golds = 255;
    _dropped_golds = static_cast<uint8_t>(factored_golds);
}

void EnemyType::apply_drop_chance_factor(double factor)
{
    if(_drop_probability > 0 && _dropped_item->id() != 1)
    {
        double factored_drop_rate = std::round(_drop_probability / factor);
        if (factored_drop_rate < 1)
            factored_drop_rate = 1;
        _drop_probability = static_cast<uint16_t>(factored_drop_rate);
    }
}

Json EnemyType::to_json() const
{
    Json json = EntityType::to_json();

    json["health"] = _health;
    json["attack"] = _attack;
    json["defence"] = _defence;
    if(_dropped_golds)
        json["droppedGolds"] = _dropped_golds;
    if(_drop_probability > 0)
    {
        json["droppedItem"] = _dropped_item->name();
        json["dropProbability"] = _drop_probability;
    }
    if(_unkillable)
        json["unkillable"] = _unkillable;

    return json;
}

void EnemyType::apply_json(const Json& json, const World& world)
{
    EntityType::apply_json(json, world);

    if(json.contains("health"))
        _health = json.at("health");
    if(json.contains("attack"))
        _attack = json.at("attack");
    if(json.contains("defence"))
        _defence = json.at("defence");
    if(json.contains("droppedGolds"))
        _dropped_golds = json.at("droppedGolds");
    if(json.contains("droppedItem"))
    {
        std::string dropped_item_name = json.at("droppedItem");
        _dropped_item = world.item(dropped_item_name);
    }
    if(json.contains("dropProbability"))
        _drop_probability = json.at("dropProbability");
    if(json.contains("unkillable"))
        _unkillable = json.at("unkillable");
}
