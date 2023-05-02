#pragma once

#include "../tools/json.hpp"
#include <sstream>
#include <utility>

class Item
{
private:
    uint8_t     _id = 0xFF;
    std::string _name;
    uint8_t     _max_quantity = 0;
    uint8_t     _starting_quantity = 0;
    uint16_t    _gold_value = 0;
    uint8_t     _verb_on_use = 0;

    // Both the following attributes require the `PatchImproveItemUseHandling` patch to be edited inside the ROM
    uint32_t    _pre_use_address = 0;   ///< Address of the "pre-use" function called when trying to use this item
    uint32_t    _post_use_address = 0;  ///< Address of the "post-use" function called after successfully using this item

public:
    Item() = default;
    Item(uint8_t id, std::string name, uint8_t max_quantity, uint8_t starting_quantity, uint16_t gold_value,
         uint8_t verb_on_use, uint32_t pre_use_addr = 0, uint32_t post_use_addr = 0) :
        _id                 (id),
        _name               (std::move(name)),
        _max_quantity       (max_quantity),
        _starting_quantity  (starting_quantity),
        _gold_value         (gold_value),
        _verb_on_use        (verb_on_use),

        _pre_use_address    (pre_use_addr),
        _post_use_address   (post_use_addr)
    {}
    
    [[nodiscard]] uint8_t id() const { return _id; }
    Item& id(uint8_t id) { _id = id; return *this; }

    [[nodiscard]] const std::string& name() const { return _name; }
    Item& name(const std::string& name) { _name = name; return *this; }

    [[nodiscard]] uint8_t starting_quantity() const { return std::min(_starting_quantity, _max_quantity); }
    Item& starting_quantity(uint8_t quantity) { _starting_quantity = quantity; return *this; }
    
    [[nodiscard]] uint8_t max_quantity() const { return _max_quantity; }
    Item& max_quantity(uint8_t quantity) { _max_quantity = quantity; return *this; }

    [[nodiscard]] uint16_t gold_value() const { return _gold_value; }
    virtual Item& gold_value(uint16_t value) { _gold_value = value; return *this; }

    [[nodiscard]] uint8_t verb_on_use() const { return _verb_on_use; }
    virtual Item& verb_on_use(uint8_t value) { _verb_on_use = value; return *this; }

    [[nodiscard]] uint32_t pre_use_address() const { return _pre_use_address; }
    virtual Item& pre_use_address(uint32_t addr) { _pre_use_address = addr; return *this; }

    [[nodiscard]] uint32_t post_use_address() const { return _post_use_address; }
    virtual Item& post_use_address(uint32_t addr) { _post_use_address = addr; return *this; }

    [[nodiscard]] Json to_json() const;
    static Item* from_json(uint8_t id, const Json& json);
    void apply_json(const Json& json);
};

////////////////////////////////////////////////////////////////

class ItemGolds : public Item
{
public:
    ItemGolds(uint8_t id, uint16_t gold_value) : 
        Item(id, format_gold_name(gold_value), 0, 0, gold_value, 0)
    {}

    Item& gold_value(uint16_t value) override
    {
        this->name(std::to_string(value) + " golds");
        return Item::gold_value(value);
    }

private:
    static std::string format_gold_name(uint16_t value)
    {
        std::string name = std::to_string(value);
        if(value > 1)
            name += " Golds";
        else
            name += " Gold";
        return name;
    }
};
