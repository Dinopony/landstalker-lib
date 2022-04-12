#include "../md_tools.hpp"

static void improve_gfx_tile_swap_flag_check(md::ROM& rom)
{
    // We need to inject a procedure checking "is D0 equal to FF" to replace the "bmi" previously used which was
    // preventing from checking flags above 0x80 (the one we need to check is 0xD0).
    md::Code proc_extended_flag_check;

    proc_extended_flag_check.moveb(addr_(reg_A0, 0x2), reg_D0);     // 1028 0002
    proc_extended_flag_check.cmpib(0xFF, reg_D0);
    proc_extended_flag_check.bne("jump_2");
    proc_extended_flag_check.jmp(0x4E2E);
    proc_extended_flag_check.label("jump_2");
    proc_extended_flag_check.jmp(0x4E20);

    uint32_t addr = rom.inject_code(proc_extended_flag_check);

    // Replace the (move.b, bmi, ext.w) by a jmp to the injected procedure
    rom.set_code(0x004E18, md::Code().clrw(reg_D0).jmp(addr));
}

static void improve_visited_flag_handling(md::ROM& rom)
{
    // Make the visited flags lookup indexed on FF1000 instead of FF10C0 to be able to set story flags
    // with visited rooms. The WorldRomReader class reads flags accordingly by adding 0xC0 to the flag byte,
    // while WorldRomWriter writes the flag as-is to fit the new system.
    rom.set_long(0x2954, 0xFF1000);
}

static void make_ingame_timer_more_precise(md::ROM& rom)
{
    // A NTSC Megadrive runs at 59.92275 FPS instead of 60 FPS as expected by the game
    // A minute lasts 3595 frames, not 3600
    rom.set_word(0x121C, 0xE0B);
}

/**
 * Add a soft reset function when A, B and C are pressed along Start to open game menu
 */
static void add_soft_reset(md::ROM& rom)
{
    md::Code check_buttons_and_reset;
    check_buttons_and_reset.cmpib(0xF0, addr_(0xFF0F8E));
    check_buttons_and_reset.bne("regular_menu_opening");

    check_buttons_and_reset.trap(0, { 0x00, 0xFE });
    check_buttons_and_reset.movew(0x10, reg_D0);
    check_buttons_and_reset.jsr(0xB7A); // Sleep
    check_buttons_and_reset.jmp(0x4B8); // ResetGame function

    check_buttons_and_reset.label("regular_menu_opening");
    check_buttons_and_reset.btst(2, addr_(0xFF1153));
    check_buttons_and_reset.beq("can_open_menu");

    check_buttons_and_reset.jmp(0x77A4); // Cannot open menu, go to return of initial function

    check_buttons_and_reset.label("can_open_menu");
    check_buttons_and_reset.jmp(0x763A); // Can open menu, get back inside initial function

    uint32_t addr = rom.inject_code(check_buttons_and_reset);
    rom.set_code(0x762E, md::Code().jmp(addr).nop(3));
}

void improve_engine(md::ROM& rom)
{
    improve_gfx_tile_swap_flag_check(rom);
    improve_visited_flag_handling(rom);
    make_ingame_timer_more_precise(rom);
    add_soft_reset(rom);
}
