#pragma once

#include <utility>
#include "item.hpp"
#include "../tools/color_palette.hpp"

class World;

class EntityType
{
private:
    uint8_t _id;
    std::string _name;
    EntityLowPalette _low_palette {};
    EntityHighPalette _high_palette {};

public:
    explicit EntityType(uint8_t id) :
        _id     (id),
        _name   ("No" + std::to_string(id))
    {}

    EntityType(uint8_t id, std::string name) :
        _id     (id),
        _name   (std::move(name))
    {}

    [[nodiscard]] virtual std::string type_name() const { return "entity"; };

    [[nodiscard]] uint8_t id() const { return _id; }

    [[nodiscard]] virtual std::string name() const { return _name; }
    void name(const std::string& name) { _name = name; }

    [[nodiscard]] bool has_low_palette() const { return _low_palette.is_valid(); }
    [[nodiscard]] const EntityLowPalette& low_palette() const { return _low_palette; }
    [[nodiscard]] EntityLowPalette& low_palette() { return _low_palette; }
    void low_palette(EntityLowPalette palette) { _low_palette = palette; }
    void clear_low_palette() { _low_palette.clear(); }

    [[nodiscard]] bool has_high_palette() const { return _high_palette.is_valid(); }
    [[nodiscard]] const EntityHighPalette& high_palette() const { return _high_palette; }
    [[nodiscard]] EntityHighPalette& high_palette() { return _high_palette; }
    void high_palette(EntityHighPalette colors) { _high_palette = colors; }
    void clear_high_palette() { _high_palette.clear(); }

    [[nodiscard]] virtual Json to_json() const
    {
        Json json;

        json["name"] = _name;
        json["type"] = this->type_name();
        if(this->has_low_palette())
            json["paletteLow"] = _low_palette.to_json();
        if(this->has_high_palette())
            json["paletteHigh"] = _high_palette.to_json();

        return json;
    }

    static EntityType* from_json(uint8_t id, const Json& json, const World& world);

    virtual void apply_json(const Json& json, const World& world)
    {
        if(json.contains("name"))
            _name = json.at("name");
    }
};

class EnemyType : public EntityType
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
    EnemyType(uint8_t id, const std::string& name,
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
    EntityItemOnGround(uint8_t id, Item* item) :
        EntityType  (id, "ground_item"),
        _item       (item)
    {}

    [[nodiscard]] std::string type_name() const override { return "ground_item"; };

    [[nodiscard]] std::string name() const override { return "ground_item (" + _item->name() + ")"; }

    [[nodiscard]] Item* item() const { return _item; }

    [[nodiscard]] Json to_json() const override
    {
        Json json = EntityType::to_json();
        json["itemId"] = _item->id();
        return json;
    }
};
