#include "../md_tools.hpp"
#include "../model/world.hpp"
#include "../model/map.hpp"
#include "../model/map_layout.hpp"
#include "../io/io.hpp"

#include <iostream>

constexpr uint16_t LINE_SIZE_IN_WORDS = 74;
constexpr uint16_t LINE_SIZE_IN_BYTES = LINE_SIZE_IN_WORDS * 2;

/**
 * At A1, set D0 consecutive words' value to D1
 * @param rom
 * @return
 */
static uint32_t inject_func_set_longs(md::ROM& rom)
{
    md::Code func_set_longs;
    {
        func_set_longs.label("loop");
        {
            func_set_longs.movel(reg_D1, addr_postinc_(reg_A1));
        }
        func_set_longs.dbra(reg_D0, "loop");
    }
    func_set_longs.rts();

    return rom.inject_code(func_set_longs);
}

/**
 * D1.w = offset from block start to reach first non-empty line
 * D2.w = offset to apply from chunk start to first non-empty data in chunk
 * D3.w = words to copy for each line (before going to next line)
 * D4.w = number of lines
 * A0 = data to copy
 * A1 = block where to copy data
 *
 * @param rom
 * @param world
 */
static uint32_t inject_func_load_data_block(md::ROM& rom)
{
    md::Code func_load_data_block;
    func_load_data_block.movem_to_stack({}, { reg_A3 });
    func_load_data_block.clrl(reg_D6);

    func_load_data_block.adda(reg_D1, reg_A1); // advance to first non-empty line
    func_load_data_block.label("line");
    {
        // Store line starting address (A1) inside A3
        func_load_data_block.movel(reg_A1, reg_A3);
        {
            func_load_data_block.adda(reg_D2, reg_A1); // advance to first non-empty word in line
            func_load_data_block.movew(reg_D3, reg_D0); // setup the amount of words to copy inside D0

            // copy D0 words from A0 to A1, or until a 0xFFFF is found
            func_load_data_block.label("loop");
            {
                // If there are still words to skip, skip it!
                func_load_data_block.tstw(reg_D6);
                func_load_data_block.bne("skip");

                func_load_data_block.movew(addr_postinc_(reg_A0), reg_D7);
                func_load_data_block.bmi("minus_case");
                func_load_data_block.movew(reg_D7, addr_postinc_(reg_A1));
            }
            func_load_data_block.label("next_loop_iteration");
            func_load_data_block.dbra(reg_D0, "loop");
        }
        func_load_data_block.movel(reg_A3, reg_A1);
        func_load_data_block.adda(LINE_SIZE_IN_BYTES, reg_A1); // advance to next line start
    }
    func_load_data_block.bra("line");

    func_load_data_block.label("ret");
    func_load_data_block.movem_from_stack({}, { reg_A3 });
    func_load_data_block.rts();

    // Code block handling the case where we need to skip iterations
    func_load_data_block.label("skip");
    func_load_data_block.subiw(1, reg_D6);
    func_load_data_block.adda(2, reg_A1);
    func_load_data_block.bra("next_loop_iteration");

    // Code block handling the case where the msb is set
    func_load_data_block.label("minus_case");
    // "10000000 00000000" case: block ending
    func_load_data_block.andiw(0x7FFF, reg_D7);
    func_load_data_block.beq("ret");

    // Cut the upper part to determine the operand
    func_load_data_block.movew(reg_D7, reg_D6);
    func_load_data_block.lsrw(8, reg_D6);
    func_load_data_block.lsrw(4, reg_D6);
    // "1000AAAA AAAAAAAA" case: skip A words
    func_load_data_block.cmpib(0x0, reg_D6); // TODO: Useful? Doesn't lsr set Z flag?
    func_load_data_block.bne("not_skip");
    {
        func_load_data_block.movew(reg_D7, reg_D6);  // Setup the amount of words to skip
        func_load_data_block.bra("skip");            // Start the skipping loop
    }
    func_load_data_block.label("not_skip");

    // "1001XXXX XXXXXXXX" case: ????
    // "1010AAAA BBBBBBBB" case: repeat A times byte B
    // "1011AAAA AAAAAAAA" case: repeat next word A times
    // "11BBBBBB AAAAAAAA" case: put byte B, then byte A


    return rom.inject_code(func_load_data_block);
}

/**
 * A2 = address of the map layout to load
 *
 * @param rom
 * @param func_load_data_block
 * @param func_set_longs
 * @return
 */
static uint32_t inject_func_load_map(md::ROM& rom, uint32_t func_load_data_block, uint32_t func_set_longs)
{
    md::Code func_load_map;
    func_load_map.movem_to_stack({ reg_D0_D7 }, { reg_A0, reg_A1 });
    {
        func_load_map.clrl(reg_D0);

        // Clear foreground & background with 0x0000
        // FIXME: Perf --> 5476 MOVEL + 5476 DBRA
        func_load_map.movel(0x00000000, reg_D1);
        func_load_map.lea(0xFF7C02, reg_A1);
        func_load_map.movew(5476, reg_D0); // 5476 longs
        func_load_map.jsr(func_set_longs);

        // Clear heightmap with 0x4000
        // FIXME: Perf --> 2738 MOVEL + 2738 DBRA
        func_load_map.movel(0x40004000, reg_D1);
        func_load_map.lea(0xFFD192, reg_A1);
        func_load_map.movew(5476 / 2, reg_D0); // 2738 longs
        func_load_map.jsr(func_set_longs);

        // --------------------------------------------------------------------------------------------------------

        // Read metadata for tiles
        func_load_map.clrl(reg_D1);
        func_load_map.moveb(addr_(reg_A2), reg_D1); // = layout->top() = 9 = amount of big chunks to skip
        func_load_map.lslw(1, reg_D1);
        func_load_map.mulu(bval_(LINE_SIZE_IN_WORDS), reg_D1); // D1 = offset from block start to reach first non-empty line

        func_load_map.clrl(reg_D2);
        func_load_map.moveb(addr_(reg_A2, 1), reg_D2); // = layout->left() = 8 = amount of small words to skip
        func_load_map.lslw(1, reg_D2); // D2 = offset to apply from chunk start to first non-empty data in chunk

        func_load_map.clrl(reg_D3);
        func_load_map.moveb(addr_(reg_A2, 2), reg_D3); // = layout->width() = 14 = words to copy for each line (before going to next chunk)
        func_load_map.subib(1, reg_D3);

        // Make A0 point on the beginning of actual data
        func_load_map.movel(reg_A2, reg_A0);
        func_load_map.adda(6, reg_A0);

        // Copy foreground
        func_load_map.lea(0xFF7C02, reg_A1);
        func_load_map.jsr(func_load_data_block);

        // Copy background
        func_load_map.lea(0xFFA6CA, reg_A1);
        func_load_map.jsr(func_load_data_block);

        // --------------------------------------------------------------------------------------------------------

        // Read metadata for heightmap
        func_load_map.clrl(reg_D1);
        func_load_map.moveb(addr_(reg_A2, 3), reg_D1); // = heightmap_top
        func_load_map.lslw(1, reg_D1);
        func_load_map.mulu(bval_(LINE_SIZE_IN_WORDS), reg_D1); // D1 = offset from block start to reach first non-empty line in heightmap

        func_load_map.clrl(reg_D2);
        func_load_map.moveb(addr_(reg_A2, 4), reg_D2); // = heightmap_left
        func_load_map.lslw(1, reg_D2); // D2 = offset to apply from chunk start to first non-empty data in chunk

        func_load_map.moveb(addr_(reg_A2, 5), reg_D3); // = layout->heightmap_width()
        func_load_map.subib(1, reg_D3);

        // Copy heightmap
        func_load_map.lea(0xFFD192, reg_A1);
        func_load_map.jsr(func_load_data_block);
    }
    func_load_map.movem_from_stack({ reg_D0_D7 }, { reg_A0, reg_A1 });
    func_load_map.rts();

    return rom.inject_code(func_load_map);
}

static void evaluate_total_size(md::ROM& rom, World& world)
{
    std::map<uint32_t, MapLayout*> all_map_layouts;
    for(auto it : world.maps())
    {
        Map* map = it.second;
        uint32_t addr = map->address();
        if(!all_map_layouts.count(addr))
            all_map_layouts[addr] = io::decode_map_layout(rom, addr);
    }

    ByteArray full_data;
    for(auto it : all_map_layouts)
    {
        MapLayout* layout = it.second;
        full_data.add_bytes(io::encode_map_layout(layout));
    }

    std::cout << "Full map data for all " << all_map_layouts.size() << " layouts takes " << full_data.size()/1000 << "KB" << std::endl;
    std::ofstream dump("./map_enc_dump.bin", std::ios::binary);
    dump.write((const char*)&(full_data[0]), (long)full_data.size());
    dump.close();
}

void new_map_format(md::ROM& rom, World& world)
{
    evaluate_total_size(rom, world);

    // Read layouts from the ROM
    std::map<uint32_t, MapLayout*> unique_map_layouts;
    for(uint16_t map_id = 601 ; map_id <= 609 ; ++map_id)
    {
        uint32_t addr = world.map(map_id)->address();
        if(!unique_map_layouts.count(addr))
            unique_map_layouts[addr] = io::decode_map_layout(rom, addr);
    }

    // Remove all vanilla map layouts from the ROM
    rom.mark_empty_chunk(0xA2392, 0x11C926);

    // Re-encode map layouts inside the ROM
    for(auto it : unique_map_layouts)
    {
        uint32_t old_addr = it.first;
        MapLayout* layout = it.second;
        uint32_t new_addr = rom.inject_bytes(io::encode_map_layout(layout));

        for(auto it2 : world.maps())
        {
            Map* map = it2.second;
            if(map->address() == old_addr)
                map->address(new_addr);
        }
    }

    uint32_t func_set_longs = inject_func_set_longs(rom);
    uint32_t func_load_data_block = inject_func_load_data_block(rom);

    uint32_t func_load_map = inject_func_load_map(rom, func_load_data_block, func_set_longs);
    std::cout << "func_load_map = 0x" << std::hex << func_load_map << std::dec << std::endl;

    rom.set_code(0x2BC8, md::Code().jmp(func_load_map));
}