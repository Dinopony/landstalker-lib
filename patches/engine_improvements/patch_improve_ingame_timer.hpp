#pragma once

#include "../game_patch.hpp"

/**
 * This patch fixes some imprecision with in-game timer.
 * A NTSC Megadrive runs at 59.92275 FPS instead of 60 FPS as expected by the game, so we need to make a minute
 * lasts 3595 frames for the timer, not 3600.
 */

class PatchImproveInGameTimer : public GamePatch
{
public:
    void alter_rom(md::ROM& rom) override
    {
        rom.set_word(0x121C, 0xE0B); // 0xE0B = 3595
    }
};
