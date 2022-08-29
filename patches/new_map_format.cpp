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
static uint32_t inject_func_clear_map_data(md::ROM& rom)
{
    md::Code func;
    {
        func.lea(0xFFFC5A, reg_A1);

        // Clear heightmap with 0x4000
        func.movel(0x40004000, reg_D1);
        func.movel(reg_D1, reg_D2);
        func.movel(reg_D1, reg_D3);
        func.movel(reg_D1, reg_D4);
        func.movel(reg_D1, reg_D5);
        func.movel(reg_D1, reg_D6);
        func.movel(reg_D1, reg_D7);
        func.movel(reg_D1, reg_A0);
        func.movel(342-1, reg_D0);
        func.label("loop_hm");
        {
            func.movem({ reg_D1, reg_D2, reg_D3, reg_D4, reg_D5, reg_D6, reg_D7 }, { reg_A0 }, true, reg_A1, md::Size::LONG);
        }
        func.dbra(reg_D0, "loop_hm");
        for(size_t i=0 ; i<2 ; ++i)
            func.movel(reg_D1, addr_predec_(reg_A1));

        // Clear foreground & background with 0x0000
        func.clrl(reg_D1);
        func.movel(reg_D1, reg_D2);
        func.movel(reg_D1, reg_D3);
        func.movel(reg_D1, reg_D4);
        func.movel(reg_D1, reg_D5);
        func.movel(reg_D1, reg_D6);
        func.movel(reg_D1, reg_D7);
        func.movel(reg_D1, reg_A0);
        func.movel(684-1, reg_D0);
        func.label("loop_tiles");
        {
            func.movem({ reg_D1, reg_D2, reg_D3, reg_D4, reg_D5, reg_D6, reg_D7 }, { reg_A0 }, true, reg_A1, md::Size::LONG);
        }
        func.dbra(reg_D0, "loop_tiles");
        for(size_t i=0 ; i<4 ; ++i)
            func.movel(reg_D1, addr_predec_(reg_A1));

        func.clrl(reg_D0);
    }
    func.rts();

    return rom.inject_code(func);
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
    md::Code func;
    func.movem_to_stack({ reg_D0_D7 }, { reg_A3 });

    func.adda(reg_D1, reg_A1); // advance to first non-empty line
    func.label("line");
    {
        // Store line starting address (A1) inside A3
        func.movel(reg_A1, reg_A3);
        {
            func.adda(reg_D2, reg_A1); // advance to first non-empty word in line
            func.movew(reg_D3, reg_D0); // setup the amount of words to copy inside D0

            // copy D0 words from A0 to A1, or until a 0xFFFF is found
            func.label("loop");
            {
                // If there are still words to skip, skip it!
                func.tstw(reg_D5);
                func.bne("skip");
                func.tstw(reg_D4);
                func.bne("repeat");

                func.movew(addr_postinc_(reg_A0), reg_D7);
                func.bmi("minus_case");
                func.movew(reg_D7, addr_postinc_(reg_A1));
            }
            func.label("next_loop_iteration");
            func.dbra(reg_D0, "loop");
        }
        func.movel(reg_A3, reg_A1);
        func.adda(LINE_SIZE_IN_BYTES, reg_A1); // advance to next line start
    }
    func.bra("line");

    func.label("ret");
    func.movem_from_stack({ reg_D0_D7 }, { reg_A3 });
    func.rts();

    // -----------------------------------------------
    // Code block handling the case where we need to skip iterations
    func.label("skip");
    {
        func.subqw(1, reg_D5);
        func.addql(2, reg_A1);
        func.bra("next_loop_iteration");
    }

    // -----------------------------------------------
    // Code block handling the case where we need to repeat data
    func.label("repeat");
    {
        func.subqw(1, reg_D4);
        func.movew(reg_D6, addr_postinc_(reg_A1));
        func.bra("next_loop_iteration");
    }

    // -----------------------------------------------
    // Code block handling the case where the msb is set
    func.label("minus_case");
    {
        func.andiw(0x7FFF, reg_D7); // Remove the topmost bit which is always set
        func.beq("ret");            // "10000000 00000000" case: end of block

        // Split the operand (D7) from the data (D6)
        func.movew(reg_D7, reg_D6);
        func.andiw(0x0FFF, reg_D6);
        func.lsll(4, reg_D7);
        func.clrw(reg_D7);
        func.swap(reg_D7);

        // Branch depending on the operand type
        func.beq("init_skip");                  // 0 --> 68710 usages
        func.cmpib(2, reg_D7);
        func.beq("init_repeat_byte");           // 2 --> 40787 usages
        func.cmpib(1, reg_D7);
        func.beq("unpack_bytes");               // 1 --> 30821 usages
        func.cmpib(5, reg_D7);
        func.beq("init_repeat_inverted_word");  // 5 --> 20744 usages
        func.cmpib(6, reg_D7);
        func.beq("init_repeat_small_word");     // 6 --> 16096 usages
        func.bra("init_repeat_word");           // 3 --> 114 usages
        //func.cmpib(3, reg_D7);
        //func.beq("init_repeat_word");
    }

    // -----------------------------------------------
    // 0 -> "1000AAAA AAAAAAAA" case: skip A words
    func.label("init_skip");
    {
        func.movew(reg_D6, reg_D5);  // Setup the amount of words to skip
        func.bra("skip");            // Start the skipping loop
    }

    // -----------------------------------------------
    // 2 -> "1010AAAA BBBBBBBB" case: repeat A times byte B
    func.label("init_repeat_byte");
    {
        // Setup amount of repeats (D4)
        func.movew(reg_D6, reg_D4);
        func.lsrw(8, reg_D4);
        // Setup word to repeat (D6)
        func.andiw(0x00FF, reg_D6);
        func.bra("repeat");            // Start the repeat loop
    }

    // -----------------------------------------------
    // 3 -> "1011AAAA AAAAAAAA" case: repeat next word A times
    func.label("init_repeat_word");
    {
        // Setup amount of repeats (D4)
        func.movew(reg_D6, reg_D4);
        // Setup word to repeat (D6)
        func.movew(addr_postinc_(reg_A0), reg_D6);
        func.bra("repeat");            // Start the repeat loop
    }

    // -----------------------------------------------
    // 5 -> "1101BBBB BBBBAAAA" case: repeat A+2 times byte B as an inverted word (0x26 >>> 0x2600)
    func.label("init_repeat_inverted_word");
    {
        // Setup amount of repeats (D4)
        func.movew(reg_D6, reg_D4);
        func.andiw(0x000F, reg_D4);
        func.addqb(2, reg_D4);
        // Setup word to repeat (D6)
        func.lslw(4, reg_D6);
        func.andiw(0xFF00, reg_D6);
        func.bra("repeat");     // Start the repeat loop
    }

    // -----------------------------------------------
    // 6 -> "1110WWWW WWWWWWXX" case: repeat 2+X times small word W
    func.label("init_repeat_small_word");
    {
        // Setup amount of repeats (D4)
        func.movew(reg_D6, reg_D4);
        func.andiw(0x0003, reg_D4);
        func.addqb(2, reg_D4);
        // Setup word to repeat (D6)
        func.lsrw(2, reg_D6);
        func.bra("repeat");     // Start the repeat loop
    }

    // -----------------------------------------------
    // 4 -> "1001AAAA AABBBBBB" case: place byte A then byte B as words
    func.label("unpack_bytes");
    {
        func.movew(reg_D6, reg_D7);
        func.lsrw(6, reg_D6);
        func.movew(reg_D6, addr_postinc_(reg_A1));
        // Setup a 1 occurence repeat to make the whole process handle line changes correctly
        func.moveq(1, reg_D4); // 1 repeat
        func.movew(reg_D7, reg_D6);
        func.andiw(0x003F, reg_D6);
        func.bra("next_loop_iteration");
    }

    return rom.inject_code(func);
}

/**
 * A2 = address of the map layout to load
 *
 * @param rom
 * @param func_load_data_block
 * @param func_set_longs
 * @return
 */
static uint32_t inject_func_load_map(md::ROM& rom, uint32_t func_load_data_block, uint32_t func_clear_map_data)
{
    md::Code func_load_map;
    func_load_map.movem_to_stack({ reg_D0_D7 }, { reg_A0, reg_A1 });
    {
        func_load_map.jsr(func_clear_map_data);

        // --------------------------------------------------------------------------------------------------------

        // Read metadata for tiles
        func_load_map.moveb(addr_(reg_A2), reg_D1); // = layout->top() = 9 = amount of big chunks to skip
        func_load_map.lslw(1, reg_D1);
        func_load_map.mulu(bval_(LINE_SIZE_IN_WORDS), reg_D1); // D1 = offset from block start to reach first non-empty line

        func_load_map.moveb(addr_(reg_A2, 1), reg_D2); // = layout->left() = 8 = amount of small words to skip
        func_load_map.lslw(1, reg_D2); // D2 = offset to apply from chunk start to first non-empty data in chunk

        func_load_map.moveb(addr_(reg_A2, 2), reg_D3); // = layout->width() = 14 = words to copy for each line (before going to next chunk)
        func_load_map.subqb(1, reg_D3);

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
        func_load_map.subqb(1, reg_D3);

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

    uint32_t func_clear_map_data = inject_func_clear_map_data(rom);
    uint32_t func_load_data_block = inject_func_load_data_block(rom);

    uint32_t func_load_map = inject_func_load_map(rom, func_load_data_block, func_clear_map_data);

    std::cout << "func_load_map = 0x" << std::hex << func_load_map << std::dec << std::endl;
    std::cout << "func_load_data_block = 0x" << std::hex << func_load_data_block << std::dec << std::endl;

    rom.set_code(0x2BC8, md::Code().jmp(func_load_map));
}