#pragma once

#include "../game_patch.hpp"

/**
 * Pawn Ticket usage is very slow in vanilla game. This patch speeds it up to make it bearable.
 */
class PatchDisableRegionCheck : public GamePatch
{
public:
    void alter_rom(md::ROM& rom) override
    {
        // Before : jsr $A0A0C | After : nop nop nop
        rom.set_code(0x506, md::Code().nop(3));
    }
};
