#include "../md_tools.hpp"

#include "../model/world.hpp"
#include "../model/map.hpp"
#include "../model/item.hpp"
#include "../constants/offsets.hpp"
#include "../constants/item_codes.hpp"
#include "../exceptions.hpp"

static void remove_alternate_palette_for_knl(const World& world)
{
    // Replace the "dark room" palette from King Nole's Labyrinth by the lit room palette
    MapPalette* lit_knl_palette = world.map_palettes().at(39);
    MapPalette* dark_knl_palette = world.map_palettes().at(40);

    for(auto& [map_id, map] : world.maps())
        if(map->palette() == dark_knl_palette)
            map->palette(lit_knl_palette);
}

static uint32_t inject_func_and_words(md::ROM& rom)
{
    // Apply an AND mask (stored in D0) on D1 words starting from A0
    md::Code func_and_words;
    func_and_words.movem_to_stack({ reg_D2 }, {});
    func_and_words.label("loop");
    func_and_words.movew(addr_(reg_A0), reg_D2);
    func_and_words.andw(reg_D0, reg_D2);
    func_and_words.movew(reg_D2, addr_(reg_A0));
    func_and_words.adda(0x2, reg_A0);
    func_and_words.dbra(reg_D1, "loop");
    func_and_words.movem_from_stack({ reg_D2 }, {});
    func_and_words.rts();
    return rom.inject_code(func_and_words);
}

static uint32_t inject_func_darken_palettes(md::ROM& rom, uint32_t func_and_words)
{
    // Darken palettes with AND filter contained in D0
    md::Code func_darken_palettes;
    func_darken_palettes.movem_to_stack({ reg_D1 }, { reg_A0 });

    // Turn palette 0, 1, and lower half of 3 pitch black
    func_darken_palettes.movew(0x0000, reg_D0);
    func_darken_palettes.lea(0xFF0080, reg_A0);
    func_darken_palettes.movew(32, reg_D1);
    func_darken_palettes.jsr(func_and_words);
    func_darken_palettes.lea(0xFF00E0, reg_A0);
    func_darken_palettes.movew(8, reg_D1);
    func_darken_palettes.jsr(func_and_words);

    // Darken palette 2 (Nigel, chests, platforms...)
//    func_darken_palettes.lea(0xFF00C0, reg_A0);
//    func_darken_palettes.movew(16, reg_D1);
//    func_darken_palettes.movew(0x0444, reg_D0);
//    func_darken_palettes.jsr(func_and_words);

    func_darken_palettes.movem_from_stack({ reg_D1 }, { reg_A0 });
    func_darken_palettes.rts();
    return rom.inject_code(func_darken_palettes);
}

static uint32_t inject_func_dim_palettes(md::ROM& rom, uint32_t func_and_words)
{
    // Darken palettes with AND filter contained in D0
    md::Code func_dim_palettes;
    func_dim_palettes.movem_to_stack({ reg_D1 }, { reg_A0 });
    // Slightly dim palette 0
    func_dim_palettes.movew(0x0CCC, reg_D0);
    func_dim_palettes.lea(0xFF0080, reg_A0);
    func_dim_palettes.movew(16, reg_D1);
    func_dim_palettes.jsr(func_and_words);
    func_dim_palettes.movem_from_stack({ reg_D1 }, { reg_A0 });
    func_dim_palettes.rts();
    return rom.inject_code(func_dim_palettes);
}

static uint32_t inject_dark_rooms_table(md::ROM& rom, const World& world)
{
    // Inject dark rooms as a data block
    ByteArray bytes;
    for (uint16_t map_id : world.dark_maps())
        bytes.add_word(map_id);
    bytes.add_word(0xFFFF);

    return rom.inject_bytes(bytes);
}

static uint32_t inject_func_darken_if_missing_lantern(md::ROM& rom, uint32_t dark_rooms_table, uint32_t func_darken_palettes, uint32_t func_dim_palettes)
{
    md::Code func_darken_if_missing_lantern;
    func_darken_if_missing_lantern.movem_to_stack({ reg_D0 }, { reg_A0 });

    // Put back white color where appropriate in all palettes to ensure it's there before an eventual darkening
    func_darken_if_missing_lantern.movew(0x0CCC, reg_D0);
    func_darken_if_missing_lantern.movew(reg_D0, addr_(0xFF0082));
    func_darken_if_missing_lantern.movew(reg_D0, addr_(0xFF00A2));
    func_darken_if_missing_lantern.movew(reg_D0, addr_(0xFF00E2));

    func_darken_if_missing_lantern.lea(dark_rooms_table, reg_A0);

    // Try to find current room in dark rooms list
    func_darken_if_missing_lantern.label("loop_start");
    func_darken_if_missing_lantern.movew(addr_(reg_A0), reg_D0);
    func_darken_if_missing_lantern.bmi("return");
    func_darken_if_missing_lantern.cmpw(addr_(0xFF1204), reg_D0);
    func_darken_if_missing_lantern.beq("current_room_is_dark");
    func_darken_if_missing_lantern.addql(0x2, reg_A0);
    func_darken_if_missing_lantern.bra("loop_start");

    // We are in a dark room
    func_darken_if_missing_lantern.label("current_room_is_dark");
    func_darken_if_missing_lantern.moveb(0xFE, addr_(0xFF112F));
    func_darken_if_missing_lantern.btst(0x1, addr_(0xFF104D));
    func_darken_if_missing_lantern.bne("owns_lantern");

    // Dark room with no lantern ===> make palettes fully black
    func_darken_if_missing_lantern.jsr(func_darken_palettes);
    func_darken_if_missing_lantern.bra("return");

    // Dark room with lantern ===> very slightly dim the palettes
    func_darken_if_missing_lantern.label("owns_lantern");
    func_darken_if_missing_lantern.jsr(func_dim_palettes);

    func_darken_if_missing_lantern.label("return");
    func_darken_if_missing_lantern.movem_from_stack({ reg_D0 }, { reg_A0 });
    func_darken_if_missing_lantern.rts();

    return rom.inject_code(func_darken_if_missing_lantern);
}

static void simplify_function_change_map_palette(md::ROM& rom, uint32_t func_darken_if_missing_lantern)
{
    md::Code func_change_map_palette;

    func_change_map_palette.cmpb(addr_(0xFF112F), reg_D4);
    func_change_map_palette.beq("return");
    func_change_map_palette.moveb(reg_D4, addr_(0xFF112F));
    func_change_map_palette.movel(addr_(offsets::MAP_PALETTES_TABLE_POINTER), reg_A0);
    func_change_map_palette.mulu(bval_(26), reg_D4);
    func_change_map_palette.adda(reg_D4, reg_A0);
    func_change_map_palette.lea(0xFF0084, reg_A1);
    func_change_map_palette.movew(0x000C, reg_D0);
    func_change_map_palette.jsr(0x96A);
    func_change_map_palette.clrw(addr_(0xFF009E));
    func_change_map_palette.clrw(addr_(0xFF0080));
    func_change_map_palette.jsr(func_darken_if_missing_lantern);
    func_change_map_palette.label("return");
    func_change_map_palette.rts();

    rom.set_code(0x2D64, func_change_map_palette);
    if(func_change_map_palette.size() > 106)
        throw LandstalkerException("func_change_map_palette is bigger than the original one");
}

/**
 * Make the lantern a flexible item, by actually being able to impact a predefined
 * table of "dark maps" and turning screen to pitch black if it's not owned.
 */
void alter_lantern_handling(md::ROM& rom, const World& world)
{
    remove_alternate_palette_for_knl(world);

    // Remove all code related to old lantern usage
    rom.mark_empty_chunk(0x87AA, 0x8832);
    world.item(ITEM_LANTERN)->pre_use_address(0);

    uint32_t dark_rooms_table = inject_dark_rooms_table(rom, world);
    uint32_t and_words = inject_func_and_words(rom);
    uint32_t darken_palettes = inject_func_darken_palettes(rom, and_words);
    uint32_t dim_palettes = inject_func_dim_palettes(rom, and_words);
    uint32_t darken_if_missing_lantern = inject_func_darken_if_missing_lantern(rom, dark_rooms_table, darken_palettes, dim_palettes);

    simplify_function_change_map_palette(rom, darken_if_missing_lantern);

    // Remove the fixed addition of white in the InitPalettes function to prevent parasitic whites to appear when
    // leaving menu or loading a saved game
    rom.set_code(0x8FFA, md::Code().nop(4));
    rom.set_code(0x9006, md::Code().nop(2));

    // ----------------------------------------
    // Hook procedure used to call the lantern check after initing all palettes (both map & entities)
    md::Code proc_init_palettes;
    proc_init_palettes.add_bytes(rom.get_bytes(0x19520, 0x19526)); // Add the jsr that was removed for injection
    proc_init_palettes.jsr(darken_if_missing_lantern);
    proc_init_palettes.rts();
    uint32_t addr = rom.inject_code(proc_init_palettes);
    rom.set_code(0x19520, md::Code().jsr(addr));
}
