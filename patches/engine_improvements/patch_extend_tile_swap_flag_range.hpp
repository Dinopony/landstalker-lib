#pragma once

#include "../game_patch.hpp"

/**
 * In the vanilla game, tile swaps (dynamic map changes reacting to flags, such as opened doors or teleporters appearing)
 * can only check flags up to 0xFF1080. This patch extends this range to 0xFF10FE.
 */
class PatchExtendTileSwapFlagRange : public GamePatch
{
public:
    void inject_code(md::ROM& rom, World& world) override
    {
        // We need to inject a procedure checking "is D0 equal to FF" to replace the "bmi" previously used which was
        // preventing from checking flags above 0x80.
        md::Code proc_extended_flag_check;
        {
            proc_extended_flag_check.moveb(addr_(reg_A0, 0x2), reg_D0);     // 1028 0002
            proc_extended_flag_check.cmpib(0xFF, reg_D0);
            proc_extended_flag_check.bne("jump_2");
            {
                proc_extended_flag_check.jmp(0x4E2E);
            }
            proc_extended_flag_check.label("jump_2");
            proc_extended_flag_check.jmp(0x4E20);
        }
        uint32_t addr = rom.inject_code(proc_extended_flag_check);

        // Replace the (move.b, bmi, ext.w) by a jmp to the injected procedure
        rom.set_code(0x004E18, md::Code().clrw(reg_D0).jmp(addr));
    }
};
