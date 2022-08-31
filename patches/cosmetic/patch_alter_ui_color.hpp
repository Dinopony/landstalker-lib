#pragma once

#include "../game_patch.hpp"
#include "../../tools/color.hpp"

/**
 * This patch changes the color of the user interface of the game (purple by default)
 */
class PatchAlterUIColor : public GamePatch
{
private:
    Color _ui_color;

public:
    explicit PatchAlterUIColor(Color ui_color) : _ui_color(ui_color) {}

    void alter_rom(md::ROM& rom) override
    {
        rom.set_word(0xF6D0, _ui_color.to_bgr_word());
        rom.set_word(0xFB36, _ui_color.to_bgr_word());
        rom.set_word(0x903C, _ui_color.to_bgr_word());
    }
};

