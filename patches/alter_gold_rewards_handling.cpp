#include "../md_tools.hpp"

#include "../constants/item_codes.hpp"
#include "../tools/byte_array.hpp"
#include "../model/item.hpp"
#include "../model/world.hpp"

static uint32_t inject_gold_rewards_block(md::ROM& rom, const World& world)
{
    ByteArray byte_array;

    for(uint32_t id=ITEM_GOLDS_START ; id < world.items().size() ; ++id)
    {
        Item* gold_item = world.item(id);
        byte_array.add_byte(static_cast<uint8_t>(gold_item->gold_value()));
    }

    return rom.inject_bytes(byte_array);
}

static uint32_t inject_function_check_gold_reward(md::ROM& rom, uint32_t gold_rewards_block)
{
    // ------------- Function to put gold reward value in D0 ----------------
    // Input: D0 = gold reward ID (offset from 0x40)
    // Output: D0 = gold reward value
    md::Code func_get_gold_reward;

    func_get_gold_reward.movem_to_stack({}, { reg_A0 });
    func_get_gold_reward.lea(gold_rewards_block, reg_A0);
    func_get_gold_reward.moveb(addr_(reg_A0, reg_D0, md::Size::WORD), reg_D0);  // move.b (A0, D0.w), D0 : 1030 0000
    func_get_gold_reward.movem_from_stack({}, { reg_A0 });
    func_get_gold_reward.rts();

    return rom.inject_code(func_get_gold_reward);
}

static void handle_gold_rewards_for_chest(md::ROM& rom, uint32_t function_check_gold_reward)
{
    rom.set_byte(0x0070DF, ITEM_GOLDS_START); // cmpi 3A, D0 >>> cmpi 40, D0
    rom.set_byte(0x0070E5, ITEM_GOLDS_START); // subi 3A, D0 >>> subi 40, D0

    // Set the call to the injected function
    // Before:      add D0,D0   ;   move.w (PC, D0, 42), D0
    // After:       jsr to injected function
    rom.set_code(0x0070E8, md::Code().jsr(function_check_gold_reward));
}

static void handle_gold_rewards_for_npc(md::ROM& rom, uint32_t function_check_gold_reward)
{
    constexpr uint32_t JUMP_ADDR_ITEM_REWARD = 0x28ECA;
    constexpr uint32_t JUMP_ADDR_GOLD_REWARD = 0x28EF0;
    constexpr uint32_t JUMP_ADDR_NO_REWARD = 0x28ED2;

    md::Code proc_check_if_item_is_golds;

    proc_check_if_item_is_golds.clrl(reg_D0);
    proc_check_if_item_is_golds.movew(addr_(0xFF1196), reg_D0);
    proc_check_if_item_is_golds.cmpiw(ITEM_GOLDS_START, reg_D0);
    proc_check_if_item_is_golds.blt("item_not_golds");
        // Gold reward case
        proc_check_if_item_is_golds.subiw(ITEM_GOLDS_START, reg_D0);
        proc_check_if_item_is_golds.jsr(function_check_gold_reward);
        proc_check_if_item_is_golds.movel(reg_D0, addr_(0xFF1878));
        proc_check_if_item_is_golds.jmp(JUMP_ADDR_GOLD_REWARD);
    proc_check_if_item_is_golds.label("item_not_golds");
    proc_check_if_item_is_golds.cmpiw(ITEM_NONE, reg_D0);
    proc_check_if_item_is_golds.bne("item_not_empty");
        // No item case
        proc_check_if_item_is_golds.movew(0x1D, reg_D0);
        proc_check_if_item_is_golds.jmp(JUMP_ADDR_NO_REWARD);
    proc_check_if_item_is_golds.label("item_not_empty");
    // Item reward case
    proc_check_if_item_is_golds.jmp(JUMP_ADDR_ITEM_REWARD);

    uint32_t addr = rom.inject_code(proc_check_if_item_is_golds);
    rom.set_code(0x28EC4, md::Code().jmp(addr));
}

/**
 * In the original game, only 3 item IDs are reserved for gold rewards (3A, 3B, 3C)
 * Here, we moved the table of gold rewards to the end of the ROM so that we can handle 64 rewards up to 255 golds each.
 * In the new system, all item IDs after the "empty item" one (0x40 and above) are now gold rewards.
 */
void alter_gold_rewards_handling(md::ROM& rom, const World& world)
{
    uint32_t gold_rewards_block = inject_gold_rewards_block(rom, world);
    uint32_t function_check_gold_reward = inject_function_check_gold_reward(rom, gold_rewards_block);
    handle_gold_rewards_for_chest(rom, function_check_gold_reward);
    handle_gold_rewards_for_npc(rom, function_check_gold_reward);
}
