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
    func_load_data_block.movem_to_stack({ reg_D0_D7 }, { reg_A3 });
    func_load_data_block.clrl(reg_D6);
    func_load_data_block.clrl(reg_D5);
    func_load_data_block.clrl(reg_D4);

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
                func_load_data_block.tstw(reg_D5);
                func_load_data_block.bne("skip");
                func_load_data_block.tstw(reg_D4);
                func_load_data_block.bne("repeat");

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
    func_load_data_block.movem_from_stack({ reg_D0_D7 }, { reg_A3 });
    func_load_data_block.rts();

    // Code block handling the case where we need to skip iterations
    func_load_data_block.label("skip");
    func_load_data_block.subiw(1, reg_D5);
    func_load_data_block.addql(2, reg_A1);
    func_load_data_block.bra("next_loop_iteration");

    // Code block handling the case where we need to repeat data
    func_load_data_block.label("repeat");
    func_load_data_block.subiw(1, reg_D4);
    func_load_data_block.movew(reg_D6, addr_postinc_(reg_A1));
    func_load_data_block.bra("next_loop_iteration");

    // Code block handling the case where the msb is set
    func_load_data_block.label("minus_case");
    // "10000000 00000000" case: block ending
    func_load_data_block.andiw(0x7FFF, reg_D7);
    func_load_data_block.beq("ret");

    // Split the operand (D7) from the data (D6)
    func_load_data_block.movew(reg_D7, reg_D6);
    func_load_data_block.andiw(0x0FFF, reg_D6);
    func_load_data_block.lsrw(8, reg_D7);
    func_load_data_block.lsrw(4, reg_D7);
    func_load_data_block.bne("not_skip");
    {
        // "1000AAAA AAAAAAAA" case: skip A words
        func_load_data_block.movew(reg_D6, reg_D5);  // Setup the amount of words to skip
        func_load_data_block.bra("skip");            // Start the skipping loop
    }
    func_load_data_block.label("not_skip");
    func_load_data_block.cmpib(0x2, reg_D7);
    func_load_data_block.bne("not_repeat_byte");
    {
        // "1010AAAA BBBBBBBB" case: repeat A times byte B
        // Setup amount of repeats (D4)
        func_load_data_block.movew(reg_D6, reg_D4);
        func_load_data_block.lsrw(8, reg_D4);
        // Setup word to repeat (D6)
        func_load_data_block.andiw(0x00FF, reg_D6);
        func_load_data_block.bra("repeat");            // Start the repeat loop
    }
    func_load_data_block.label("not_repeat_byte");
    func_load_data_block.cmpib(0x3, reg_D7);
    func_load_data_block.bne("not_repeat_word");
    {
        // "1011AAAA AAAAAAAA" case: repeat next word A times
        // Setup amount of repeats (D4)
        func_load_data_block.movew(reg_D6, reg_D4);
        // Setup word to repeat (D6)
        func_load_data_block.movew(addr_postinc_(reg_A0), reg_D6);
        func_load_data_block.bra("repeat");            // Start the repeat loop
    }
    func_load_data_block.label("not_repeat_word");
    func_load_data_block.cmpib(0x1, reg_D7);
    func_load_data_block.bne("not_packed_bytes");
    {
        // "1001AAAA AABBBBBB" case: place byte A then byte B as words
        func_load_data_block.movew(reg_D6, reg_D7);
        func_load_data_block.lsrw(6, reg_D6);
        func_load_data_block.movew(reg_D6, addr_postinc_(reg_A1));
        // Setup a 1 occurence repeat to make the whole process handle line changes correctly
        func_load_data_block.movew(1, reg_D4); // 1 repeat
        func_load_data_block.movew(reg_D7, reg_D6);
        func_load_data_block.andiw(0x003F, reg_D6);
        func_load_data_block.bra("next_loop_iteration");
    }
    func_load_data_block.label("not_packed_bytes");



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
        func_load_map.addql(6, reg_A0);

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

void new_map_format(md::ROM& rom, World& world)
{
    // Read layouts from the ROM
    std::map<uint32_t, MapLayout*> unique_map_layouts;
    for(auto it : world.maps())
    {
        uint32_t addr = it.second->address();
        if(!unique_map_layouts.count(addr))
            unique_map_layouts[addr] = io::decode_map_layout(rom, addr);
    }

    // Remove all vanilla map layouts from the ROM
    rom.mark_empty_chunk(0xA2392, 0x11C926);

    uint32_t total_size = 0;

    // Re-encode map layouts inside the ROM
    for(auto it : unique_map_layouts)
    {
        uint32_t old_addr = it.first;
        MapLayout* layout = it.second;

        ByteArray bytes = io::encode_map_layout(layout);
        total_size += bytes.size();
        uint32_t new_addr = rom.inject_bytes(bytes);

        std::ofstream dump("./map_layouts/map_" + std::to_string(old_addr) + ".bin", std::ios::binary);
        dump.write((const char*)&(bytes[0]), (long)bytes.size());
        dump.close();

        for(auto it2 : world.maps())
        {
            uint16_t map_id = it2.first;
            Map* map = it2.second;
            if(map->address() == old_addr)
            {
                map->address(new_addr);
//                std::cout << "Map #" << map_id << " address is " << old_addr << std::endl;
            }
        }
    }

    std::cout << "Full map data for all " << unique_map_layouts.size() << " layouts takes " << total_size/1000 << "KB" << std::endl;

    uint32_t func_set_longs = inject_func_set_longs(rom);
    uint32_t func_load_data_block = inject_func_load_data_block(rom);

    uint32_t func_load_map = inject_func_load_map(rom, func_load_data_block, func_set_longs);

    std::cout << "func_load_map = 0x" << std::hex << func_load_map << std::dec << std::endl;
    std::cout << "func_load_data_block = 0x" << std::hex << func_load_data_block << std::dec << std::endl;

    rom.set_code(0x2BC8, md::Code().jmp(func_load_map));
}