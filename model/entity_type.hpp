#pragma once

#include <utility>
#include "item.hpp"

class World;

using EntityLowPaletteColors = std::array<uint16_t, 6>;
using EntityHighPaletteColors = std::array<uint16_t, 7>;

class EntityType
{
private:
    uint8_t _id;
    std::string _name;
    EntityLowPaletteColors _low_palette { 0, 0, 0, 0, 0, 0 };
    EntityHighPaletteColors _high_palette { 0, 0, 0, 0, 0, 0, 0 };

public:
    EntityType(uint8_t id, std::string name) :
        _id     (id),
        _name   (std::move(name))
    {}

    [[nodiscard]] virtual std::string type_name() const { return "entity"; };

    [[nodiscard]] uint8_t id() const { return _id; }

    [[nodiscard]] std::string name() const { return _name; }
    void name(const std::string& name) { _name = name; }

    [[nodiscard]] bool has_low_palette() const { return _low_palette[0] != 0 && _low_palette[_low_palette.size()-1] != 0; }
    [[nodiscard]] EntityLowPaletteColors low_palette() const { return _low_palette; }
    void low_palette(EntityLowPaletteColors colors) { _low_palette = colors; }
    void clear_low_palette() { _low_palette.fill(0); }

    [[nodiscard]] bool has_high_palette() const { return _high_palette[0] != 0 && _high_palette[_high_palette.size()-1] != 0; }
    [[nodiscard]] EntityHighPaletteColors high_palette() const { return _high_palette; }
    void high_palette(EntityHighPaletteColors colors) { _high_palette = colors; }
    void clear_high_palette() { _high_palette.fill(0); }

    [[nodiscard]] virtual Json to_json() const
    {
        Json json;

        json["name"] = _name;
        json["type"] = this->type_name();
        if(this->has_low_palette())
            json["paletteLow"] = _low_palette;
        if(this->has_high_palette())
            json["paletteHigh"] = _high_palette;

        return json;
    }

    static EntityType* from_json(uint8_t id, const Json& json, const World& world);

    virtual void apply_json(const Json& json, const World& world)
    {
        if(json.contains("name"))
            _name = json.at("name");
    }
};

class EntityEnemy : public EntityType
{
private :
    uint8_t _health;
    uint8_t _attack;
    uint8_t _defence;
    uint8_t _dropped_golds;
    Item* _dropped_item;
    uint16_t _drop_probability;  ///< 128 ---> 1/128 chance
    bool _unkillable;

public:
    EntityEnemy(uint8_t id, const std::string& name,
                uint8_t health, uint8_t attack, uint8_t defence,
                uint8_t dropped_golds, Item* dropped_item, uint16_t drop_probability);

    [[nodiscard]] std::string type_name() const override { return "enemy"; };

    [[nodiscard]] uint8_t health() const { return _health; }
    void health(uint8_t health) { _health = health; }

    [[nodiscard]] uint8_t attack() const { return _attack; }
    void attack(uint8_t attack) { _attack = attack; }

    [[nodiscard]] uint8_t defence() const { return _defence; }
    void defence(uint8_t defence) { _defence = defence; }

    [[nodiscard]] uint8_t dropped_golds() const { return (_unkillable) ? 0 : _dropped_golds; }
    void dropped_golds(uint8_t dropped_golds) { _dropped_golds = dropped_golds; }

    [[nodiscard]] Item* dropped_item() const { return _dropped_item; }
    [[nodiscard]] uint8_t dropped_item_id() const { return (_unkillable) ? 0x3F : _dropped_item->id(); }
    void dropped_item(Item* dropped_item) { _dropped_item = dropped_item; }

    [[nodiscard]] uint16_t drop_probability() const { return (_unkillable) ? 1 : _drop_probability; }
    void drop_probability(uint16_t drop_probability) { _drop_probability = drop_probability; }

    [[nodiscard]] bool unkillable() const { return _unkillable; }
    void unkillable(bool value) { _unkillable = value; }

    void apply_health_factor(double factor);
    void apply_armor_factor(double factor);
    void apply_damage_factor(double factor);
    void apply_golds_factor(double factor);
    void apply_drop_chance_factor(double factor);

    [[nodiscard]] Json to_json() const override;
    void apply_json(const Json& json, const World& world) override;
};

class EntityItemOnGround : public EntityType
{
private :
    Item* _item;

public:
    EntityItemOnGround(uint8_t id, const std::string& name, Item* item) :
        EntityType  (id, name),
        _item       (item)
    {}

    [[nodiscard]] std::string type_name() const override { return "ground_item"; };

    [[nodiscard]] Item* item() const { return _item; }

    [[nodiscard]] Json to_json() const override
    {
        Json json = EntityType::to_json();
        json["itemId"] = _item->id();
        return json;
    }
};
