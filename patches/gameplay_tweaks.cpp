#include "../md_tools.hpp"

void make_lifestocks_give_specific_health(md::ROM& rom, uint8_t health_per_lifestock)
{
    uint16_t health_per_lifestock_formatted = health_per_lifestock << 8;
    rom.set_word(0x291E2, health_per_lifestock_formatted);
    rom.set_word(0x291F2, health_per_lifestock_formatted);
    rom.set_word(0x7178, health_per_lifestock_formatted);
    rom.set_word(0x7188, health_per_lifestock_formatted);
}

void remove_tree_cutting_glitch_drops(md::ROM& rom)
{
    constexpr uint32_t ADDR_DROP_ITEM_BOX = 0x162CC;
    constexpr uint32_t ADDR_DROP_NOTHING = 0x16270;

    md::Code proc_filter_sacred_tree_reward;
    proc_filter_sacred_tree_reward.cmpiw(0x126, addr_(reg_A5, 0xA));
    proc_filter_sacred_tree_reward.beq("not_a_tree");
    proc_filter_sacred_tree_reward.movew(0x0060, addr_(reg_A5, 0x3A));
    proc_filter_sacred_tree_reward.jmp(ADDR_DROP_ITEM_BOX);
    proc_filter_sacred_tree_reward.label("not_a_tree");
    proc_filter_sacred_tree_reward.jmp(ADDR_DROP_NOTHING);

    uint32_t proc_addr = rom.inject_code(proc_filter_sacred_tree_reward);
    rom.set_code(0x162C6, md::Code().jmp(proc_addr));
}
