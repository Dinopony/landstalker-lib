#include "../md_tools.hpp"
#include "../constants/item_codes.hpp"

[[nodiscard]] static uint32_t inject_function_alter_item_in_d0(md::ROM& rom)
{
    md::Code func_alter_item_in_d0;

    // Check if item ID is between 09 and 0C (armors). If not, branch to return.
    func_alter_item_in_d0.cmpib(ITEM_STEEL_BREAST, reg_D0);
    func_alter_item_in_d0.blt(13);
    func_alter_item_in_d0.cmpib(ITEM_HYPER_BREAST, reg_D0);
    func_alter_item_in_d0.bgt(11);

    // By default, put Hyper breast as given armor
    func_alter_item_in_d0.movew(ITEM_HYPER_BREAST, reg_D0);

    // If Shell breast is not owned, put Shell breast
    func_alter_item_in_d0.btst(0x05, addr_(0xFF1045));
    func_alter_item_in_d0.bne(2);
    func_alter_item_in_d0.movew(ITEM_SHELL_BREAST, reg_D0);

    // If Chrome breast is not owned, put Chrome breast
    func_alter_item_in_d0.btst(0x01, addr_(0xFF1045));
    func_alter_item_in_d0.bne(2);
    func_alter_item_in_d0.movew(ITEM_CHROME_BREAST, reg_D0);

    // If Steel breast is not owned, put Steel breast
    func_alter_item_in_d0.btst(0x05, addr_(0xFF1044));
    func_alter_item_in_d0.bne(2);
    func_alter_item_in_d0.movew(ITEM_STEEL_BREAST, reg_D0);

    func_alter_item_in_d0.rts();

    return rom.inject_code(func_alter_item_in_d0);
}

[[nodiscard]] static uint32_t inject_function_change_item_in_reward_textbox(md::ROM& rom, uint32_t function_alter_item_in_d0)
{
    md::Code func_change_item_reward_textbox;

    func_change_item_reward_textbox.jsr(function_alter_item_in_d0);
    func_change_item_reward_textbox.movew(reg_D0, addr_(0xFF1196));
    func_change_item_reward_textbox.rts();

    return rom.inject_code(func_change_item_reward_textbox);
}

[[nodiscard]] static uint32_t inject_function_alter_item_given_by_ground_source(md::ROM& rom, 
                                                                                uint32_t function_alter_item_in_d0)
{
    md::Code func_alter_item_given_by_ground_source;

    func_alter_item_given_by_ground_source.movem_to_stack({ reg_D7 }, { reg_A0 }); // movem D7,A0 -(A7)	(48E7 0180)

    func_alter_item_given_by_ground_source.cmpib(ITEM_HYPER_BREAST, reg_D0);
    func_alter_item_given_by_ground_source.bgt(9); // to movem
    func_alter_item_given_by_ground_source.cmpib(ITEM_STEEL_BREAST, reg_D0);
    func_alter_item_given_by_ground_source.blt(7);  // to movem

    func_alter_item_given_by_ground_source.jsr(function_alter_item_in_d0);
    func_alter_item_given_by_ground_source.moveb(addr_(reg_A5, 0x3B), reg_D7);  // move ($3B,A5), D7	(1E2D 003B)
    func_alter_item_given_by_ground_source.subib(0xC9, reg_D7);
    func_alter_item_given_by_ground_source.cmpa(lval_(0xFF5400), reg_A5);
    func_alter_item_given_by_ground_source.blt(2);    // to movem
    func_alter_item_given_by_ground_source.bset(reg_D7, addr_(0xFF103F)); // set a flag when an armor is taken on the ground for it to disappear afterwards

    func_alter_item_given_by_ground_source.movem_from_stack({ reg_D7 }, { reg_A0 }); // movem (A7)+, D7,A0	(4CDF 0180)
    func_alter_item_given_by_ground_source.lea(0xFF1040, reg_A0);
    func_alter_item_given_by_ground_source.rts();

    return rom.inject_code(func_alter_item_given_by_ground_source);
}

[[nodiscard]] static uint32_t inject_function_alter_visible_item_for_ground_source(md::ROM& rom, 
                                                  uint32_t function_change_item_in_reward_textbox)
{
    md::Code func_alter_visible_item_for_ground_source;

    func_alter_visible_item_for_ground_source.movem_to_stack({ reg_D7 }, { reg_A0 });  // movem D7,A0 -(A7)

    func_alter_visible_item_for_ground_source.subib(0xC0, reg_D0);
    func_alter_visible_item_for_ground_source.cmpib(ITEM_HYPER_BREAST, reg_D0);
    func_alter_visible_item_for_ground_source.bgt(10); // to move D0 in item slot
    func_alter_visible_item_for_ground_source.cmpib(ITEM_STEEL_BREAST, reg_D0);
    func_alter_visible_item_for_ground_source.blt(8); // to move D0 in item slot
    func_alter_visible_item_for_ground_source.moveb(reg_D0, reg_D7);
    func_alter_visible_item_for_ground_source.subib(ITEM_STEEL_BREAST, reg_D7);
    func_alter_visible_item_for_ground_source.btst(reg_D7, addr_(0xFF103F));
    func_alter_visible_item_for_ground_source.bne(3);
    // Item was not already taken, alter the armor inside
    func_alter_visible_item_for_ground_source.jsr(function_change_item_in_reward_textbox);
    func_alter_visible_item_for_ground_source.bra(2);
    // Item was already taken, remove it by filling it with an empty item
    func_alter_visible_item_for_ground_source.movew(ITEM_NONE, reg_D0);
    func_alter_visible_item_for_ground_source.moveb(reg_D0, addr_(reg_A1, 0x36)); // move D0, ($36,A1) (1340 0036)
    func_alter_visible_item_for_ground_source.movem_from_stack({ reg_D7 }, { reg_A0 }); // movem (A7)+, D7,A0	(4CDF 0180)
    func_alter_visible_item_for_ground_source.rts();

    return rom.inject_code(func_alter_visible_item_for_ground_source);
}

void handle_armor_upgrades(md::ROM& rom)
{
    uint32_t function_alter_item_in_d0 = inject_function_alter_item_in_d0(rom);

    uint32_t function_change_item_in_reward_textbox = inject_function_change_item_in_reward_textbox(
        rom, function_alter_item_in_d0);

    uint32_t function_alter_item_given_by_ground_source = inject_function_alter_item_given_by_ground_source(
        rom, function_alter_item_in_d0);

    uint32_t function_alter_visible_item_for_ground_source = inject_function_alter_visible_item_for_ground_source(
        rom, function_change_item_in_reward_textbox);

    // Insert the actual calls to the previously injected functions

    // In 'chest reward' function, replace the item ID move by the injected function
    rom.set_code(0x0070BE, md::Code().jsr(function_change_item_in_reward_textbox));

    // In 'NPC reward' function, replace the item ID move by the injected function
    rom.set_code(0x028DD8, md::Code().jsr(function_change_item_in_reward_textbox));

    // In 'item on ground reward' function, replace the item ID move by the injected function
    rom.set_word(0x024ADC, 0x3002); // put the move D2,D0 before the jsr because it helps us while changing nothing to the usual logic
    rom.set_code(0x024ADE, md::Code().jsr(function_change_item_in_reward_textbox));

    // Replace 2928C lea (41F9 00FF1040) by a jsr to injected function
    rom.set_code(0x02928C, md::Code().jsr(function_alter_item_given_by_ground_source));

    // Replace 1963C - 19644 (0400 00C0 ; 1340 0036) by a jsr to a replacement function
    rom.set_code(0x01963C, md::Code().jsr(function_alter_visible_item_for_ground_source).nop());
}
