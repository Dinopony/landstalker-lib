#include "../md_tools.hpp"
#include "../tools/color.hpp"

void alter_ui_color(md::ROM& rom, Color ui_color)
{
    rom.set_word(0xF6D0, ui_color.to_bgr_word());
    rom.set_word(0xFB36, ui_color.to_bgr_word());
    rom.set_word(0x903C, ui_color.to_bgr_word());
}
