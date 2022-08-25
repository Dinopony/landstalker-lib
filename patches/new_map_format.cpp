#include "../md_tools.hpp"
#include "../model/world.hpp"
#include "../model/map.hpp"
#include "../model/map_layout.hpp"
#include "../io/io.hpp"

#include <iostream>

constexpr uint16_t LINE_SIZE_IN_WORDS = 74;
constexpr uint16_t LINE_SIZE_IN_BYTES = LINE_SIZE_IN_WORDS * 2;

static ByteArray map_foreground_as_bytes(MapLayout* layout)
{
    std::vector<uint16_t> tile_values = layout->foreground_tiles();
    std::cout << "Foreground count before = " << tile_values.size() << std::endl;
    while(!tile_values.empty() && *tile_values.rbegin() == 0x0000)
        tile_values.pop_back();
    std::cout << "Foreground count after = " << tile_values.size() << std::endl << std::endl;

    ByteArray bytes;
    for(uint16_t tile : tile_values)
        bytes.add_word(tile);
    bytes.add_word(0xFFFF);
    return bytes;
}

static ByteArray map_background_as_bytes(MapLayout* layout)
{
    std::vector<uint16_t> tile_values = layout->background_tiles();
    std::cout << "Background count before = " << tile_values.size() << std::endl;
    while(!tile_values.empty() && *tile_values.rbegin() == 0x0000)
        tile_values.pop_back();
    std::cout << "Background count after = " << tile_values.size() << std::endl << std::endl;

    ByteArray bytes;
    for(uint16_t tile : tile_values)
        bytes.add_word(tile);
    bytes.add_word(0xFFFF);
    return bytes;
}

static ByteArray map_heightmap_as_bytes(MapLayout* layout)
{
    std::vector<uint16_t> tile_values = layout->heightmap();
    std::cout << "Heightmap count before = " << tile_values.size() << std::endl;
    while(!tile_values.empty() && *tile_values.rbegin() == 0x4000)
        tile_values.pop_back();
    std::cout << "Heightmap count after = " << tile_values.size() << std::endl << std::endl;

    ByteArray bytes;
    for(uint16_t tile : tile_values)
        bytes.add_word(tile);
    bytes.add_word(0xFFFF);
    return bytes;
}

static uint32_t inject_map_data(md::ROM& rom, MapLayout* layout)
{
    ByteArray data;

    data.add_byte(layout->top()); // = 9 = amount of big chunks to skip
    data.add_byte(layout->left()); // = 8 = amount of small words to skip
    data.add_byte(layout->width()); // = 14 = words to copy for each line (before going to next chunk)
    data.add_byte(0xc); // heightmap top
    data.add_byte(0xc); // heightmap left
    data.add_byte(layout->heightmap_width());

    // HOUSE
    // (0x8,0x9) | (0x0e 0x0f) ===> 0x9 + 0x3 = 0xc

    // EXTERIOR
    // (0x1,0x5) | (0x3a 0x3b) ===> 0x5 + 0x7

    data.add_bytes(map_foreground_as_bytes(layout));
    data.add_bytes(map_background_as_bytes(layout));
    data.add_bytes(map_heightmap_as_bytes(layout));

    return rom.inject_bytes(data);
}

/**
 * Copy D0 words from A0 to A1, and stop if a 0xFFFF is found
 * @param rom
 * @return
 */
static uint32_t inject_func_copy_words(md::ROM& rom)
{
    md::Code func_copy_words;
    func_copy_words.movem_to_stack({ reg_D0 }, {});
    {
        func_copy_words.clrl(reg_D7);
        func_copy_words.label("loop");
        {
            func_copy_words.movew(addr_(reg_A0), reg_D7);
            func_copy_words.adda(2, reg_A0);
            // Exit if word is a terminal word
            func_copy_words.cmpiw(0xFFFF, reg_D7);
            func_copy_words.beq("ret");
            // Otherwise, just copy it
            func_copy_words.movew(reg_D7, addr_(reg_A1));
            func_copy_words.adda(2, reg_A1);
        }
        func_copy_words.dbra(reg_D0, "loop");
    }
    func_copy_words.label("ret");
    func_copy_words.movem_from_stack({ reg_D0 }, {});
    func_copy_words.rts();

    return rom.inject_code(func_copy_words);
}

/**
 * At A1, set D0 consecutive words' value to D1
 * @param rom
 * @return
 */
static uint32_t inject_func_set_words(md::ROM& rom)
{
    md::Code func_set_words;
    {
        func_set_words.label("loop");
        {
            func_set_words.movew(reg_D1, addr_(reg_A1));
            func_set_words.adda(2, reg_A1);
        }
        func_set_words.dbra(reg_D0, "loop");
    }
    func_set_words.rts();

    return rom.inject_code(func_set_words);
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
static uint32_t inject_func_load_data_block(md::ROM& rom, uint32_t func_copy_words)
{
    md::Code func_load_data_block;

    func_load_data_block.adda(reg_D1, reg_A1); // advance to first non-empty line
    func_load_data_block.label("line");
    {
        func_load_data_block.movem_to_stack({}, { reg_A1 }); // store line start address
        {
            func_load_data_block.adda(reg_D2, reg_A1); // advance to first non-empty word in line
            func_load_data_block.clrl(reg_D0);
            func_load_data_block.moveb(reg_D3, reg_D0);
            func_load_data_block.jsr(func_copy_words); // copy D0 words from A0 to A1
        }
        func_load_data_block.movem_from_stack({}, { reg_A1 }); // restore line start address

        // Check if we left last function because of a terminal word
        func_load_data_block.cmpiw(0xFFFF, reg_D7);
        func_load_data_block.beq("ret");

        func_load_data_block.adda(LINE_SIZE_IN_BYTES, reg_A1); // advance to next line start
    }
    func_load_data_block.bra("line");

    func_load_data_block.label("ret");
    func_load_data_block.rts();

    return rom.inject_code(func_load_data_block);
}

// TODO: Use postincrement instead of adda for performance

void new_map_format(md::ROM& rom, World& world)
{
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
        uint32_t new_addr = inject_map_data(rom, layout);

        for(auto it2 : world.maps())
        {
            Map* map = it2.second;
            if(map->address() == old_addr)
                map->address(new_addr);
        }
    }

    uint32_t func_copy_words = inject_func_copy_words(rom);
    uint32_t func_set_words = inject_func_set_words(rom);
    uint32_t func_load_data_block = inject_func_load_data_block(rom, func_copy_words);

    // Input: A2 = address of the map layout
    md::Code func_load_map;
    func_load_map.movem_to_stack({ reg_D0_D7 }, { reg_A0, reg_A1 });
    {
        func_load_map.clrl(reg_D0);

        // Clear foreground & background with 0x0000
        func_load_map.movew(0x0000, reg_D1);
        func_load_map.lea(0xFF7C02, reg_A1); // Foreground
        func_load_map.movew(5476 * 2, reg_D0);
        func_load_map.jsr(func_set_words);

        // Clear heightmap with 0x4000
        func_load_map.movew(0x4000, reg_D1);
        func_load_map.lea(0xFFD192, reg_A1);
        func_load_map.movew(5476, reg_D0);
        func_load_map.jsr(func_set_words);

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
    uint32_t func_load_map_addr = rom.inject_code(func_load_map);
    std::cout << "func_load_map_addr = 0x" << std::hex << func_load_map_addr << std::dec << std::endl;

    rom.set_code(0x2BC8, md::Code().jmp(func_load_map_addr));
}