#pragma once

#include "../game_patch.hpp"
#include "../../constants/offsets.hpp"
#include "../../model/item.hpp"

/**
 * In the base game, each item can be assigned two functions :
 *  - a "pre-use" function (called while in inventory, usually used to display a message)
 *  - a "post-use" function (called when inventory is closed, typically used to trigger specific effects in the map
 *      like Garlic, Einstein Whistle or Gola's Eye)
 *
 * These functions are stored in two tables taking the form of a relative branch BRA instruction.
 * The problem with those is that you cannot extend this table without freeing space around it, because the reach
 * of a BRA instruction is pretty limited (+/- 0x7FFF bytes).
 *
 * This patch reworks the table by replacing those relative BRA by absolute JMP instructions. This makes the table
 * slightly bigger (2 bytes lost per item), but makes it way easier (= possible) to extend.
 */
class PatchImproveItemUseHandling : public GamePatch
{
public:
    void clear_space_in_rom(md::ROM& rom) override
    {
        rom.mark_empty_chunk(offsets::ITEM_PRE_USE_TABLE, offsets::ITEM_PRE_USE_TABLE_END);
        rom.mark_empty_chunk(offsets::ITEM_POST_USE_TABLE, offsets::ITEM_POST_USE_TABLE_END);
    }

    void postprocess(md::ROM& rom, const World& world) override
    {
        // Everything this patch does is done as "postprocessing" because every item needs to have its pre_use and
        // post_use attributes well configured, and this sometimes require other patches to inject code.
        // So whatever this patch does needs to be done after the "inject_code" step.
        uint32_t pre_use_table_addr = inject_pre_use_table(rom, world);
        alter_pre_use_function(rom, pre_use_table_addr);

        uint32_t post_use_table_addr = inject_post_use_table(rom, world);
        alter_post_use_function(rom, post_use_table_addr);
    }

private:
    static uint32_t inject_pre_use_table(md::ROM& rom, const World& world)
    {
        md::Code pre_use_table;
        for(auto& [id, item] : world.items())
        {
            if(!item->pre_use_address())
                continue;

            pre_use_table.jmp(item->pre_use_address());
            pre_use_table.add_byte(item->id());
            pre_use_table.add_byte(0xFF);
        }

        pre_use_table.nop(3);
        pre_use_table.add_byte(0xFF);
        pre_use_table.add_byte(0xFF);

        return rom.inject_code(pre_use_table);
    }

    static uint32_t inject_post_use_table(md::ROM& rom, const World& world)
    {
        md::Code post_use_table;
        for(auto& [id, item] : world.items())
        {
            if(!item->post_use_address())
                continue;

            post_use_table.jmp(item->post_use_address());
            post_use_table.add_byte(item->id() | 0x80);
            post_use_table.add_byte(0xFF);
        }

        post_use_table.nop(3);
        post_use_table.add_byte(0xFF);
        post_use_table.add_byte(0xFF);

        return rom.inject_code(post_use_table);
    }

    static void alter_pre_use_function(md::ROM& rom, uint32_t pre_use_table_addr)
    {
        md::Code func_set_a0;
        {
            func_set_a0.moveb(reg_D0, addr_(0xFF1152));
            func_set_a0.lea(pre_use_table_addr, reg_A0);
        }
        func_set_a0.rts();
        uint32_t addr = rom.inject_code(func_set_a0);

        rom.set_code(0x85F2, md::Code().jsr(addr).nop(2));

        rom.set_byte(0x85FF, 0x06); // Jump instructions are now 6 bytes long instead of 4 bytes long
        rom.set_code(0x860E, md::Code().addql(0x8, reg_A0)); // A block is now 8 bytes long instead of 6
    }

    static void alter_post_use_function(md::ROM& rom, uint32_t post_use_table_addr)
    {
        md::Code func_set_a0;
        {
            func_set_a0.moveb(addr_(0xFF1152), reg_D0);
            func_set_a0.lea(post_use_table_addr, reg_A0);
        }
        func_set_a0.rts();
        uint32_t addr = rom.inject_code(func_set_a0);

        rom.set_code(0x8BC8, md::Code().jsr(addr).nop(2));

        rom.set_byte(0x8BD5, 0x06); // Jump instructions are now 6 bytes long instead of 4 bytes long
        rom.set_code(0x8BE0, md::Code().addql(0x8, reg_A0)); // A block is now 8 bytes long instead of 6
    }
};
