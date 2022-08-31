#pragma once

#include "../game_patch.hpp"
#include "../../constants/offsets.hpp"

/**
 * Changes the order of items in the inventory with the one passed on construction.
 * Items absent from this list are made completely invisible.
 */
class PatchAlterInventoryOrder : public GamePatch
{
private:
    std::array<uint8_t, 40> _inventory_order;

public:
    explicit PatchAlterInventoryOrder(const std::array<uint8_t, 40>& inventory_order) : _inventory_order(inventory_order) {}

    void alter_rom(md::ROM& rom) override
    {
        uint32_t addr = offsets::MENU_ITEM_ORDER_TABLE;
        for(uint8_t item : _inventory_order)
        {
            rom.set_byte(addr, item);
            addr += 1;
        }
    }
};
