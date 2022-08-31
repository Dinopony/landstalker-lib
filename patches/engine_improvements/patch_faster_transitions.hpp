#pragma once

#include "../game_patch.hpp"

/**
 * All sorts of transitions (map transition, pause menu transition...) use a fade to black followed by a fade from
 * black. Both those fades originally take 2 frames per fade step, this patch makes those steps only 1-frame long.
 * This makes for overall faster and smoother transitions.
 */
class PatchFasterTransitions : public GamePatch
{
public:
    void alter_rom(md::ROM& rom) override
    {
        rom.set_byte(0x8F17, 0x01); // 2x faster fade to black
        rom.set_byte(0x8EE7, 0x01); // 2x faster fade from black
    }
};
