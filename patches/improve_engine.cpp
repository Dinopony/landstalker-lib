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

void improve_engine(md::ROM& rom)
{
    improve_gfx_tile_swap_flag_check(rom);
    improve_visited_flag_handling(rom);
}
