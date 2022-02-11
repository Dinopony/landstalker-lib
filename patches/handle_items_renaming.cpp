#include "../md_tools.hpp"

#include "../model/world.hpp"
#include "../model/item.hpp"
#include "../tools/byte_array.hpp"
#include "../tools/stringtools.hpp"
#include "../constants/symbols.hpp"
#include "../constants/offsets.hpp"

static uint32_t inject_item_names_table(const World& world, md::ROM& rom)
{
    ByteArray item_names_table_bytes;

    for(auto& [id, item] : world.items())
    {
        // Compute item name
        ByteArray item_name_bytes;
        std::vector<std::string> item_name_words = stringtools::split(item->name(), " ");
        uint8_t current_line_length = 0;
        for(std::string word : item_name_words)
        {
            if(current_line_length == 7 || current_line_length == 8)
            {
                current_line_length = 0;
                item_name_bytes.add_byte(0x6A);
            }

            uint8_t line_length_with_word = current_line_length + word.length();
            // If this isn't the first word on the line, adding a space before the word is required
            if(current_line_length > 0)
                line_length_with_word += 1;


            if(line_length_with_word <= 8)
            {
                if(current_line_length > 0)
                    word = " " + word;

                current_line_length = line_length_with_word;
                item_name_bytes.add_bytes(Symbols::bytes_for_symbols(word));
            }
            else if(word.length() <= 8)
            {
                item_name_bytes.add_byte(0x6A); // Line break without hyphen in inventory, space in textboxes
                current_line_length = word.length();
                item_name_bytes.add_bytes(Symbols::bytes_for_symbols(word));
            }
            else
            {
                uint8_t symbols_on_current_line = 7 - current_line_length;
                item_name_bytes.add_bytes(Symbols::bytes_for_symbols(word.substr(0, symbols_on_current_line)));
                item_name_bytes.add_byte(0x69); // Line break without hyphen in inventory, space in textboxes
                std::string remainder = word.substr(symbols_on_current_line);
                item_name_bytes.add_bytes(Symbols::bytes_for_symbols(remainder));
                current_line_length = remainder.length();
            }
        }

        item_names_table_bytes.add_byte(item_name_bytes.size());
        item_names_table_bytes.add_bytes(item_name_bytes);
    }

    return rom.inject_bytes(item_names_table_bytes);
}

static void alter_uncompressed_string_reading_function(md::ROM& rom, uint32_t name_table_addr)
{
    md::Code lea_item_name_table_pointer;
    lea_item_name_table_pointer.lea(name_table_addr, reg_A2);
    lea_item_name_table_pointer.jmp(0x294B0);
    uint32_t injected_proc_addr = rom.inject_code(lea_item_name_table_pointer);
    rom.set_code(0x294A2, md::Code().jmp(injected_proc_addr));
}

void handle_items_renaming(md::ROM& rom, const World& world)
{
    rom.mark_empty_chunk(offsets::ITEM_NAMES_TABLE, offsets::ITEM_NAMES_TABLE_END);

    uint32_t name_table_addr = inject_item_names_table(world, rom);
    alter_uncompressed_string_reading_function(rom, name_table_addr);
}
