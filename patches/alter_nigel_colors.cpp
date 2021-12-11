#include "../md_tools.hpp"
#include "../constants/offsets.hpp"

static void set_nigel_color(md::ROM& rom, uint8_t color_id, uint16_t color)
{
    uint32_t color_address = offsets::NIGEL_PALETTE + (color_id * 2);
    rom.set_word(color_address, color);

    color_address = offsets::NIGEL_PALETTE_SAVE_MENU + (color_id * 2);
    rom.set_word(color_address, color);
}

/// colors follows the (0BGR) format used by palettes
void alter_nigel_colors(md::ROM& rom, const std::pair<uint16_t, uint16_t>& nigel_colors)
{
    set_nigel_color(rom, 9, nigel_colors.first);    // Light color
    set_nigel_color(rom, 10, nigel_colors.second);  // Dark color
}
