#include "../md_tools.hpp"

#include "../constants/offsets.hpp"
#include "../constants/item_codes.hpp"

#include <array>

////////////////////////////////////////////////////////////////////////////////////////////////////////
//      UI CHANGES
////////////////////////////////////////////////////////////////////////////////////////////////////////

void alter_item_order_in_menu(md::ROM& rom, const std::array<uint8_t, 40>& inventory_order)
{
    uint32_t addr = offsets::MENU_ITEM_ORDER_TABLE;
    for(uint8_t item : inventory_order)
    {
        rom.set_byte(addr, item);
        addr += 1;
    }
}

/**
 * Make the effect of Statue of Gaia and Sword of Gaia way faster, because reasons.
 */
void quicken_gaia_effect(md::ROM& rom)
{
    constexpr uint8_t SPEEDUP_FACTOR = 3;

    rom.set_word(0x1686C, rom.get_word(0x1686C) * SPEEDUP_FACTOR);
    rom.set_word(0x16878, rom.get_word(0x16878) * SPEEDUP_FACTOR);
    rom.set_word(0x16884, rom.get_word(0x16884) * SPEEDUP_FACTOR);
}

void quicken_pawn_ticket_effect(md::ROM& rom)
{
    constexpr uint8_t SPEEDUP_FACTOR = 3;

    rom.set_word(0x8920, rom.get_word(0x8920) / SPEEDUP_FACTOR);
}

/**
 * All sorts of transitions (map transition, pause menu transition...) use a fade to black followed by a fade from
 * black. Both those fades originally take 2 frames per fade step, this patch makes those steps only 1-frame long.
 * This makes for overall faster and smoother transitions.
 */
void make_transitions_faster(md::ROM& rom)
{
    rom.set_byte(0x8F17, 0x01); // 2x faster fade to black
    rom.set_byte(0x8EE7, 0x01); // 2x faster fade from black
}
