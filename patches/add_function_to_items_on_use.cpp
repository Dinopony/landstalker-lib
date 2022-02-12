#include "../md_tools.hpp"

#include "../model/world.hpp"
#include "../model/item.hpp"
#include "../constants/item_codes.hpp"
#include "../constants/offsets.hpp"

uint32_t inject_pre_use_table(md::ROM& rom, const World& world)
{
    rom.mark_empty_chunk(offsets::ITEM_PRE_USE_TABLE, offsets::ITEM_PRE_USE_TABLE_END);

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

void alter_pre_use_function(md::ROM& rom, uint32_t pre_use_table_addr)
{
    md::Code func_set_a0;
    func_set_a0.moveb(reg_D0, addr_(0xFF1152));
    func_set_a0.lea(pre_use_table_addr, reg_A0);
    func_set_a0.rts();
    uint32_t addr = rom.inject_code(func_set_a0);

    rom.set_code(0x85F2, md::Code().jsr(addr).nop(2));

    rom.set_byte(0x85FF, 0x06); // Jump instructions are now 6 bytes long instead of 4 bytes long
    rom.set_code(0x860E, md::Code().addql(0x8, reg_A0)); // A block is now 8 bytes long instead of 6
}

uint32_t inject_post_use_table(md::ROM& rom, const World& world)
{
    rom.mark_empty_chunk(offsets::ITEM_POST_USE_TABLE, offsets::ITEM_POST_USE_TABLE_END);

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

void alter_post_use_function(md::ROM& rom, uint32_t post_use_table_addr)
{
    md::Code func_set_a0;
    func_set_a0.moveb(addr_(0xFF1152), reg_D0);
    func_set_a0.lea(post_use_table_addr, reg_A0);
    func_set_a0.rts();
    uint32_t addr = rom.inject_code(func_set_a0);

    rom.set_code(0x8BC8, md::Code().jsr(addr).nop(2));

    rom.set_byte(0x8BD5, 0x06); // Jump instructions are now 6 bytes long instead of 4 bytes long
    rom.set_code(0x8BE0, md::Code().addql(0x8, reg_A0)); // A block is now 8 bytes long instead of 6
}

void add_functions_to_items_on_use(md::ROM& rom, const World& world)
{
    uint32_t pre_use_table_addr = inject_pre_use_table(rom, world);
    alter_pre_use_function(rom, pre_use_table_addr);

    uint32_t post_use_table_addr = inject_post_use_table(rom, world);
    alter_post_use_function(rom, post_use_table_addr);
}
