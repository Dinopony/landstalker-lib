#pragma once

#include "../game_patch.hpp"

/**
 * Add a soft reset function when A, B and C are pressed along Start to open game menu
 */
class PatchAddSoftReset : public GamePatch
{
public:
    void inject_code(md::ROM& rom, World& world) override
    {
        md::Code check_buttons_and_reset;
        {
            check_buttons_and_reset.cmpib(0xF0, addr_(0xFF0F8E));
            check_buttons_and_reset.bne("regular_menu_opening");
            {
                check_buttons_and_reset.trap(0, { 0x00, 0xFE });
                check_buttons_and_reset.movew(0x10, reg_D0);
                check_buttons_and_reset.jsr(0xB7A); // Sleep
                check_buttons_and_reset.jmp(0x4B8); // ResetGame function
            }
            check_buttons_and_reset.label("regular_menu_opening");
            check_buttons_and_reset.btst(2, addr_(0xFF1153));
            check_buttons_and_reset.beq("can_open_menu");
            {
                check_buttons_and_reset.jmp(0x77A4); // Cannot open menu, go to return of initial function
            }
            check_buttons_and_reset.label("can_open_menu");
            check_buttons_and_reset.jmp(0x763A); // Can open menu, get back inside initial function
        }
        uint32_t addr = rom.inject_code(check_buttons_and_reset);
        rom.set_code(0x762E, md::Code().jmp(addr).nop(3));
    }
};
