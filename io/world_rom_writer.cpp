#include "io.hpp"

#include "../model/entity_type.hpp"
#include "../model/item_source.hpp"
#include "../model/map.hpp"
#include "../model/world.hpp"

#include "../exceptions.hpp"
#include "../constants/offsets.hpp"
#include "../constants/entity_type_codes.hpp"
#include "../tools/huffman_tree.hpp"
#include "../tools/stringtools.hpp"

#include <cstdint>
#include <set>

///////////////////////////////////////////////////////////////////////////////

static ByteArray item_name_as_bytes(Item* item)
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

    return item_name_bytes;
}

static void write_item_names(const World& world, md::ROM& rom)
{
    rom.mark_empty_chunk(offsets::ITEM_NAMES_TABLE, offsets::ITEM_NAMES_TABLE_END);

    ByteArray item_names_table_bytes;

    for(auto& [id, item] : world.items())
    {
        ByteArray item_name_bytes = item_name_as_bytes(item);
        item_names_table_bytes.add_byte(item_name_bytes.size());
        item_names_table_bytes.add_bytes(item_name_bytes);
    }

    uint32_t name_table_addr = rom.inject_bytes(item_names_table_bytes);

    md::Code lea_item_name_table_pointer;
    {
        lea_item_name_table_pointer.lea(name_table_addr, reg_A2);
        lea_item_name_table_pointer.jmp(0x294B0);
    }
    uint32_t injected_proc_addr = rom.inject_code(lea_item_name_table_pointer);
    rom.set_code(0x294A2, md::Code().jmp(injected_proc_addr));
}

static void write_items(const World& world, md::ROM& rom)
{
    // Prepare a data block for gold values
    uint8_t highest_item_id = world.items().rbegin()->first;
    uint8_t gold_items_count = (highest_item_id - ITEM_GOLDS_START) + 1;
    uint32_t gold_values_table_addr = rom.reserve_data_block(gold_items_count, "data_gold_values");

    // Write actual items
    for(auto& [item_id, item] : world.items())
    {
        // Special treatment for gold items (only write their value in the previously prepared data block)
        if(item->id() >= ITEM_GOLDS_START)
        {
            uint32_t addr = gold_values_table_addr + (item_id - ITEM_GOLDS_START);
            rom.set_byte(addr, static_cast<uint8_t>(world.items().at(item_id)->gold_value()));
            continue;
        }

        uint32_t item_base_addr = offsets::ITEM_DATA_TABLE + item_id * 0x04;

        // Set max quantity
        uint8_t verb_and_max_qty = rom.get_byte(item_base_addr);
        verb_and_max_qty &= 0xF0;
        verb_and_max_qty += item->max_quantity();
        rom.set_byte(item_base_addr, verb_and_max_qty);

        // Set gold value
        rom.set_word(item_base_addr + 0x2, item->gold_value());
    }
}

static void write_item_sources(const World& world, md::ROM& rom)
{
    for(ItemSource* source : world.item_sources())
    {
        if(source->type_name() == "chest")
        {
            uint8_t chest_id = reinterpret_cast<ItemSourceChest*>(source)->chest_id();
            rom.set_byte(0x9EABE + chest_id, source->item_id());
        }
        else if(source->type_name() == "reward")
        {
            uint32_t address_in_rom = reinterpret_cast<ItemSourceReward*>(source)->address_in_rom();
            rom.set_byte(address_in_rom, source->item_id());
        }
        else
        {
            // Ground & shop item sources are tied to map entities that are updated as their contents change.
            // Therefore those types of item sources will effectively be written when map entities are written.
            ItemSourceOnGround* ground_source = reinterpret_cast<ItemSourceOnGround*>(source);
            for (Entity* entity : ground_source->entities())
                entity->entity_type_id(ground_source->item_id() + 0xC0);
        }
    }
}

static void write_entity_types(const World& world, md::ROM& rom)
{
    std::vector<EntityEnemy*> enemy_types;
    std::set<uint16_t> drop_probabilities;
    std::map<uint16_t, uint8_t> drop_probability_lookup;
    std::map<EntityEnemy*, uint16_t> instances_in_game;

    // List all different enemy types and different drop probabilities
    // Also count the number of instances for each enemy in the game to optimize
    // stats loading in game.
    for(auto& [id, entity_type] : world.entity_types())
    {
        if(entity_type->type_name() != "enemy")
            continue;
        EntityEnemy* enemy_type = reinterpret_cast<EntityEnemy*>(entity_type);

        enemy_types.emplace_back(enemy_type);
        drop_probabilities.insert(enemy_type->drop_probability());

        uint32_t count = 0;
        for(auto& [map_id, map] : world.maps())
        {
            for(Entity* entity : map->entities())
            {
                if(entity->entity_type_id() == entity_type->id())
                    count++;
            }
        }
        instances_in_game[enemy_type] = count;
    }

    // Sort the enemy types by number of instances in game (descending)
    std::sort(enemy_types.begin(), enemy_types.end(), 
        [instances_in_game](EntityEnemy* a, EntityEnemy* b) -> bool {
            return instances_in_game.at(a) > instances_in_game.at(b);
        }
    );

    // Build a lookup table with probabilities
    if(drop_probabilities.size() > 8)
        throw LandstalkerException("Having more than 8 different loot probabilities is not allowed");

    uint8_t current_id = 0;
    uint32_t addr = offsets::PROBABILITY_TABLE;
    for(uint16_t probability : drop_probabilities)
    {
        drop_probability_lookup[probability] = current_id++;
        rom.set_word(addr, probability);
        addr += 0x2;
    }

    // Write the actual enemy stats
    addr = offsets::ENEMY_STATS_TABLE;
    for(EntityEnemy* enemy_type : enemy_types)
    {
        uint8_t byte5 = enemy_type->attack() & 0x7F;
        uint8_t byte6 = enemy_type->dropped_item_id() & 0x3F;

        uint8_t probability_id = drop_probability_lookup.at(enemy_type->drop_probability());
        byte5 |= (probability_id & 0x4) << 5;
        byte6 |= (probability_id & 0x3) << 6;

        rom.set_byte(addr, enemy_type->id());
        rom.set_byte(addr+1, enemy_type->health());
        rom.set_byte(addr+2, enemy_type->defence());
        rom.set_byte(addr+3, enemy_type->dropped_golds());
        rom.set_byte(addr+4, byte5);
        rom.set_byte(addr+5, byte6);
        addr += 0x6;
    }
    rom.set_word(addr, 0xFFFF);
    addr += 0x2;
    if(addr > offsets::ENEMY_STATS_TABLE_END)
        throw LandstalkerException("Enemy stats table is bigger than in original game");
}

static void write_entity_type_palettes(const World& world, md::ROM& rom)
{
    // Build arrays for low & high palettes
    std::vector<EntityLowPalette> low_palettes;
    std::vector<EntityHighPalette> high_palettes;

    uint32_t addr = offsets::ENTITY_PALETTES_TABLE;
    ByteArray entity_palette_uses;
    for(auto& [id, entity_type] : world.entity_types())
    {
        if(entity_type->has_low_palette())
        {
            uint8_t palette_id = low_palettes.size();
            for(size_t i=0 ; i<low_palettes.size() ; ++i)
            {
                if(low_palettes.at(i) == entity_type->low_palette())
                {
                    palette_id = i;
                    break;
                }
            }
            if(palette_id == low_palettes.size())
                low_palettes.emplace_back(entity_type->low_palette());

            entity_palette_uses.add_byte(id);
            entity_palette_uses.add_byte(palette_id & 0x7F);
        }

        if(entity_type->has_high_palette())
        {
            uint8_t palette_id = high_palettes.size();
            for(size_t i=0 ; i<high_palettes.size() ; ++i)
            {
                if(high_palettes[i] == entity_type->high_palette())
                {
                    palette_id = i;
                    break;
                }
            }
            if(palette_id == high_palettes.size())
                high_palettes.emplace_back(entity_type->high_palette());

            entity_palette_uses.add_byte(id);
            entity_palette_uses.add_byte(0x80 + (palette_id & 0x7F));
        }
    }
    entity_palette_uses.add_word(0xFFFF);

    // Write the palette uses table, referencing which entity uses which palette(s)
    if(entity_palette_uses.size() > offsets::ENTITY_PALETTES_TABLE_END - offsets::ENTITY_PALETTES_TABLE)
        throw LandstalkerException("Entity palette uses table must not be bigger than the one from base game");
    rom.set_bytes(offsets::ENTITY_PALETTES_TABLE, entity_palette_uses);

    // Write the low palettes contents
    ByteArray low_palettes_bytes;
    for(const EntityLowPalette& palette : low_palettes)
        low_palettes_bytes.add_bytes(palette.to_bytes());

    if(low_palettes_bytes.size() > offsets::ENTITY_PALETTES_TABLE_LOW_END - offsets::ENTITY_PALETTES_TABLE_LOW)
        throw LandstalkerException("Entity low palettes table must not be bigger than the one from base game");
    rom.set_bytes(offsets::ENTITY_PALETTES_TABLE_LOW, low_palettes_bytes);

    // Write the high palettes contents
    ByteArray high_palettes_bytes;
    for(const EntityHighPalette& palette : high_palettes)
        high_palettes_bytes.add_bytes(palette.to_bytes());
    if(high_palettes_bytes.size() > offsets::ENTITY_PALETTES_TABLE_HIGH_END - offsets::ENTITY_PALETTES_TABLE_HIGH)
        throw LandstalkerException("Entity high palettes table must not be bigger than the one from base game");
    rom.set_bytes(offsets::ENTITY_PALETTES_TABLE_HIGH, high_palettes_bytes);
}

static void write_game_strings(const World& world, md::ROM& rom)
{
    // Write Huffman tree offsets & tree data consecutively in the ROM
    std::vector<HuffmanTree*> huffman_trees = io::build_trees_from_strings(world.game_strings());
    ByteArray encodeded_trees = io::encode_huffman_trees(huffman_trees);

    constexpr uint32_t ORIGINAL_SIZE = 0x2469C - offsets::HUFFMAN_TREE_OFFSETS;
    if(encodeded_trees.size() > ORIGINAL_SIZE)
        throw LandstalkerException("New Huffman trees data size is " + std::to_string(encodeded_trees.size() - ORIGINAL_SIZE) + " bytes bigger than the original size");

    rom.set_bytes(offsets::HUFFMAN_TREE_OFFSETS, encodeded_trees);

    // Write textbanks to the ROM
    rom.mark_empty_chunk(offsets::FIRST_TEXTBANK, offsets::TEXTBANKS_TABLE_END);

    std::vector<ByteArray> encoded_textbanks = io::encode_textbanks(world.game_strings(), huffman_trees);
    ByteArray textbanks_table_bytes;
    for(const ByteArray& encoded_textbank : encoded_textbanks)
    {
        uint32_t textbank_addr = rom.inject_bytes(encoded_textbank);
        textbanks_table_bytes.add_long(textbank_addr);
    }
    textbanks_table_bytes.add_long(0xFFFFFFFF);

    uint32_t textbanks_table_addr = rom.inject_bytes(textbanks_table_bytes);
    rom.set_long(offsets::TEXTBANKS_TABLE_POINTER, textbanks_table_addr);

    for(HuffmanTree* tree : huffman_trees)
        delete tree;
}

static void write_fahl_enemies(const World& world, md::ROM& rom)
{
    if(world.fahl_enemies().size() > 50)
        throw LandstalkerException("Cannot put more than 50 enemies for Fahl challenge");

    ByteArray fahl_enemies_bytes;
    for(EntityType* entity_type : world.fahl_enemies())
        fahl_enemies_bytes.add_byte(entity_type->id());

    rom.set_bytes(offsets::FAHL_ENEMIES_TABLE, fahl_enemies_bytes);
}

static void write_map_connections(const World& world, md::ROM& rom)
{
    ByteArray map_connection_bytes;

    for(const MapConnection& connection : world.map_connections())
    {
        uint8_t byte_1 = (connection.map_id_1() >> 8) & 0x03;
        byte_1 |= (connection.extra_byte_1() << 2) & 0xFC;
        uint8_t byte_2 = connection.map_id_1() & 0xFF;
        uint8_t byte_3 = connection.pos_x_1();
        uint8_t byte_4 = connection.pos_y_1();

        uint8_t byte_5 = (connection.map_id_2() >> 8) & 0x03;
        byte_5 |= (connection.extra_byte_2() << 2) & 0xFC;
        uint8_t byte_6 = connection.map_id_2() & 0xFF;
        uint8_t byte_7 = connection.pos_x_2();
        uint8_t byte_8 = connection.pos_y_2();

        map_connection_bytes.add_bytes({ byte_1, byte_2, byte_3, byte_4, byte_5, byte_6, byte_7, byte_8 });
    }

    map_connection_bytes.add_word(0xFFFF);

    rom.mark_empty_chunk(offsets::MAP_CONNECTIONS_TABLE, offsets::MAP_CONNECTIONS_TABLE_END);
    uint32_t new_map_connections_table_addr = rom.inject_bytes(map_connection_bytes);
    rom.set_long(offsets::MAP_CONNECTIONS_TABLE_POINTER, new_map_connections_table_addr);
}

static void write_map_palettes(const World& world, md::ROM& rom)
{
    ByteArray palette_table_bytes;

    for(const MapPalette* palette : world.map_palettes())
    {
        for(const Color& color : *palette)
            palette_table_bytes.add_word(color.to_bgr_word());
    }

    rom.mark_empty_chunk(offsets::MAP_PALETTES_TABLE, offsets::MAP_PALETTES_TABLE_END);
    uint32_t new_palette_table_addr = rom.inject_bytes(palette_table_bytes);
    rom.set_long(offsets::MAP_PALETTES_TABLE_POINTER, new_palette_table_addr);
}

static void write_blocksets(const World& world, md::ROM& rom)
{
    rom.mark_empty_chunk(offsets::BLOCKSETS_GROUPS_TABLE, offsets::SOUND_BANK);
    ByteArray blockset_groups_table;

    std::map<Blockset*, uint32_t> blockset_addresses;
    for(auto& group : world.blockset_groups())
    {
        ByteArray blockset_group_bytes;
        for(Blockset* blockset : group)
        {
            if(!blockset_addresses.count(blockset))
            {
                ByteArray blockset_bytes = io::encode_blockset(blockset);
                blockset_addresses[blockset] = rom.inject_bytes(blockset_bytes);
            }

            blockset_group_bytes.add_long(blockset_addresses[blockset]);
        }
        uint32_t blockset_group_addr = rom.inject_bytes(blockset_group_bytes);
        blockset_groups_table.add_long(blockset_group_addr);
    }

    uint32_t blockset_groups_table_addr = rom.inject_bytes(blockset_groups_table);
    rom.set_long(offsets::BLOCKSETS_GROUPS_TABLE_POINTER, blockset_groups_table_addr);
}

static std::map<MapLayout*, uint32_t> write_map_layouts(const World& world, md::ROM& rom)
{
    // Remove all vanilla map layouts from the ROM
    rom.mark_empty_chunk(offsets::MAP_LAYOUTS_START, offsets::MAP_LAYOUTS_END);

//    uint32_t total_size = 0;

    std::map<MapLayout*, uint32_t> layout_addresses;

    for(MapLayout* layout : world.map_layouts())
    {
        ByteArray bytes = io::encode_map_layout(layout);
//        total_size += bytes.size();

        uint32_t addr = rom.inject_bytes(bytes);
        layout_addresses[layout] = addr;
    }

//    std::cout << "Full map data for all " << world.map_layouts().size() << " layouts takes " << total_size/1000 << "KB" << std::endl;

    return layout_addresses;
}


///////////////////////////////////////////////////////////////////////////////

static void write_maps_data(const World& world, md::ROM& rom, const std::map<MapLayout*, uint32_t>& map_layout_adresses)
{
    ByteArray map_data_table;

    for(const auto& [map_id, map] : world.maps())
    {
        std::pair<uint8_t, uint8_t> blockset_id = world.blockset_id(map->blockset());

        uint8_t byte4;
        byte4 = blockset_id.first & 0x3F;
        byte4 |= (map->unknown_param_1() & 0x03) << 6;

        uint8_t byte5;
        byte5 = world.map_palette_id(map->palette()) & 0x3F;
        byte5 |= (map->unknown_param_2() & 0x03) << 6;

        uint8_t byte7;
        byte7 = map->background_music() & 0x1F;
        byte7 |= ((blockset_id.second-1) & 0x07) << 5;

        uint32_t map_layout_addr = 0xFFFFFFFF;
        if(map_layout_adresses.count(map->layout()))
            map_layout_addr = map_layout_adresses.at(map->layout());

        map_data_table.add_long(map_layout_addr);
        map_data_table.add_byte(byte4);
        map_data_table.add_byte(byte5);
        map_data_table.add_byte(map->room_height());
        map_data_table.add_byte(byte7);
    }

    rom.mark_empty_chunk(offsets::MAP_DATA_TABLE, offsets::MAP_DATA_TABLE_END);
    uint32_t map_data_table_addr = rom.inject_bytes(map_data_table);
    rom.set_long(offsets::MAP_DATA_TABLE_POINTER, map_data_table_addr);
}

static void write_maps_visited_flag(const World& world, md::ROM& rom)
{
    ByteArray map_visited_flag_table;

    for(auto& [map_id, map] : world.maps())
    {
        uint16_t flag_word = (map->visited_flag().byte << 3) + map->visited_flag().bit;
        map_visited_flag_table.add_word(flag_word);
    }

    rom.mark_empty_chunk(offsets::MAP_VISITED_FLAG_TABLE, offsets::MAP_VISITED_FLAG_TABLE_END);
    uint32_t map_visited_flag_table_addr = rom.inject_bytes(map_visited_flag_table);
    rom.set_long(offsets::MAP_VISITED_FLAG_TABLE_POINTER, map_visited_flag_table_addr);
}

static void write_maps_base_chest_id(const World& world, md::ROM& rom)
{
    ByteArray map_base_chest_id_table;

    for(auto& [map_id, map] : world.maps())
    {
        map_base_chest_id_table.add_byte(map->base_chest_id());
    }

    rom.mark_empty_chunk(offsets::MAP_BASE_CHEST_ID_TABLE, offsets::MAP_BASE_CHEST_ID_TABLE_END);
    uint32_t map_base_chest_id_table_addr = rom.inject_bytes(map_base_chest_id_table);

    // Extend the appropriate function to go get the new table instead of the old one
    md::Code func;
    {
        // Loading a save gives an invalid chest ID for the first chest because map ID stored in D0 was preceded
        // by FFFF when loading, and 0000 during normal gameplay. Using a bitmask makes the behavior consistent.
        func.andil(0x0000FFFF, reg_D0);

        func.clrw(reg_D1);
        func.lea(map_base_chest_id_table_addr, reg_A0);
        func.moveb(addr_(reg_A0, reg_D0), reg_D1);
        func.rts();
    }
    uint32_t extended_base_chest_id_handler = rom.inject_code(func);
    rom.set_code(0x9E76A, md::Code().jsr(extended_base_chest_id_handler));
}

static void write_maps_fall_destination(const World& world, md::ROM& rom)
{
    uint32_t addr = offsets::MAP_FALL_DESTINATION_TABLE;

    for(auto& [map_id, map] : world.maps())
    {
        uint16_t fall_destination = map->fall_destination();
        if(fall_destination != 0xFFFF)
        {
            rom.set_word(addr, map_id);
            rom.set_word(addr+2, fall_destination);
            addr += 0x4;
        }
    }

    rom.set_word(addr, 0xFFFF);
    addr += 0x2;
    if(addr > offsets::MAP_FALL_DESTINATION_TABLE_END)
        throw LandstalkerException("Fall destination table must not be bigger than the one from base game");
}

static void write_maps_climb_destination(const World& world, md::ROM& rom)
{
    uint32_t addr = offsets::MAP_CLIMB_DESTINATION_TABLE;

    for(auto& [map_id, map] : world.maps())
    {
        uint16_t climb_destination = map->climb_destination();
        if(climb_destination != 0xFFFF)
        {
            rom.set_word(addr, map_id);
            rom.set_word(addr+2, climb_destination);
            addr += 0x4;
        }
    }

    rom.set_word(addr, 0xFFFF);
    addr += 0x2;
    if(addr > offsets::MAP_CLIMB_DESTINATION_TABLE_END)
        throw LandstalkerException("Climb destination table must not be bigger than the one from base game");
}

static void write_maps_variants(const World& world, md::ROM& rom)
{
    uint32_t addr = offsets::MAP_VARIANTS_TABLE;

    std::map<std::pair<uint16_t, uint16_t>, Flag> variant_flags;
    for(auto& [map_id, map] : world.maps())
    {
        for(auto& [variant_map, flag] : map->variants())
            variant_flags[std::make_pair(map->id(), variant_map->id())] = flag;
    }

    // We process variants reversed so that when several variants are available,
    // the last one in the list is the one coming from the "origin" map. This is especially
    // important since the game only defines map exits for the origin map (not the variants)
    // and the algorithm it uses to find back the origin map takes the last find in the list.
    for(auto it = variant_flags.rbegin() ; it != variant_flags.rend() ; ++it)
    {
        uint16_t map_id = it->first.first;
        uint16_t variant_map_id = it->first.second;
        const Flag& flag = it->second;

        rom.set_word(addr, map_id);
        rom.set_word(addr+2, variant_map_id);
        rom.set_byte(addr+4, (uint8_t)flag.byte);
        rom.set_byte(addr+5, flag.bit);
        addr += 0x6;
    }

    rom.set_long(addr, 0xFFFFFFFF);
    addr += 0x4;
    if(addr > offsets::MAP_VARIANTS_TABLE_END)
        throw LandstalkerException("Map variants must not be bigger than the one from base game");
    rom.mark_empty_chunk(addr, offsets::MAP_VARIANTS_TABLE_END);
}

static void write_maps_global_entity_masks(const World& world, md::ROM& rom)
{
    uint32_t addr = offsets::MAP_CLEAR_FLAGS_TABLE;

    for(auto& [map_id, map] : world.maps())
    {
        for(const GlobalEntityMaskFlag& global_mask_flags : map->global_entity_mask_flags())
        {
            uint16_t flag_bytes = global_mask_flags.to_bytes();
            rom.set_word(addr, map->id());
            rom.set_word(addr+2, flag_bytes);
            addr += 0x4;
        }
    }

    rom.set_word(addr, 0xFFFF);
    addr += 0x2;
    if(addr > offsets::MAP_CLEAR_FLAGS_TABLE_END)
        throw LandstalkerException("Map clear flags table must not be bigger than the one from base game");
    rom.mark_empty_chunk(addr, offsets::MAP_CLEAR_FLAGS_TABLE_END);
}

static void write_maps_key_door_masks(const World& world, md::ROM& rom)
{
    uint32_t addr = offsets::MAP_CLEAR_KEY_DOOR_FLAGS_TABLE;

    for(auto& [map_id, map] : world.maps())
    {
        for(const GlobalEntityMaskFlag& global_mask_flags : map->key_door_mask_flags())
        {
            uint16_t flag_bytes = global_mask_flags.to_bytes();
            rom.set_word(addr, map->id());
            rom.set_word(addr+2, flag_bytes);
            addr += 0x4;
        }
    }

    rom.set_word(addr, 0xFFFF);
    addr += 0x2;
    if(addr > offsets::MAP_CLEAR_KEY_DOOR_FLAGS_TABLE_END)
        throw LandstalkerException("Map key door clear flags table must not be bigger than the one from base game");
    rom.mark_empty_chunk(addr, offsets::MAP_CLEAR_KEY_DOOR_FLAGS_TABLE_END);
}

static void write_maps_entity_masks(const World& world, md::ROM& rom)
{
    ByteArray entity_masks_table;

    for(auto& [map_id, map] : world.maps())
    {
        std::vector<uint8_t> bytes;

        uint8_t entity_id = 0;
        for(Entity* entity : map->entities())
        {
            for(EntityMaskFlag& mask_flag : entity->mask_flags())
            {
                uint8_t flag_msb = mask_flag.byte & 0x7F;
                if(mask_flag.visibility_if_flag_set)
                    flag_msb |= 0x80;

                uint8_t flag_lsb = entity_id & 0x0F;
                flag_lsb |= (mask_flag.bit & 0x7) << 5;

                entity_masks_table.add_word(map_id);
                entity_masks_table.add_byte(flag_msb);
                entity_masks_table.add_byte(flag_lsb);
            }
            entity_id++;
        }
    }

    entity_masks_table.add_word(0xFFFF);

    rom.mark_empty_chunk(offsets::MAP_ENTITY_MASKS_TABLE, offsets::MAP_ENTITY_MASKS_TABLE_END);
    uint32_t entity_masks_table_addr = rom.inject_bytes(entity_masks_table);

    // Modify pointer leading to this table, since its address has changed
    md::Code func_setup_entity_masks_pointer;
    {
        func_setup_entity_masks_pointer.lea(entity_masks_table_addr, reg_A0);
        func_setup_entity_masks_pointer.lea(0xFF1000, reg_A1);
        func_setup_entity_masks_pointer.rts();
    }
    uint32_t setup_addr = rom.inject_code(func_setup_entity_masks_pointer);
    rom.set_code(0x1A382, md::Code().jsr(setup_addr).nop(2));
}

static void write_maps_dialogue_table(const World& world, md::ROM& rom)
{
    ByteArray bytes;

    for(auto& [map_id, map] : world.maps())
    {
        const std::vector<uint16_t>& map_speakers = map->speaker_ids();

        std::vector<std::pair<uint16_t, uint8_t>> consecutive_packs;

        uint16_t previous_speaker_id = 0xFFFE;
        uint8_t current_dialogue_id = -1;
        // Assign "dialogue" sequentially to entities
        for(uint16_t speaker_id : map_speakers)
        {
            if(speaker_id != previous_speaker_id)
            {
                if(speaker_id == previous_speaker_id + 1)
                    consecutive_packs[consecutive_packs.size()-1].second++;
                else
                    consecutive_packs.emplace_back(std::make_pair(speaker_id, 1));

                previous_speaker_id = speaker_id;
                current_dialogue_id++;
            }
        }

        if(!consecutive_packs.empty())
        {
            // Write map header announcing how many dialogue words follow
            uint16_t header_word = map_id + ((uint16_t)consecutive_packs.size() << 11);
            bytes.add_word(header_word);

            // Write speaker IDs in map
            for(auto& pair : consecutive_packs)
            {
                uint16_t speaker_id = pair.first;
                uint8_t consecutive_speakers = pair.second;

                uint16_t pack_word = (speaker_id & 0x7FF) + (consecutive_speakers << 11);
                bytes.add_word(pack_word);
            }
        }
    }
    bytes.add_word(0xFFFF);

    rom.mark_empty_chunk(offsets::DIALOGUE_TABLE, offsets::DIALOGUE_TABLE_END);
    uint32_t room_dialogue_table_addr = rom.inject_bytes(bytes);

    // Inject a small function in order to load dialogue table address in A0 wherever it is in the ROM
    md::Code code;
    {
        code.movew(addr_(0xFF1206), reg_D0);
        code.lea(room_dialogue_table_addr, reg_A0);
        code.rts();
    }
    uint32_t injected_func = rom.inject_code(code);
    rom.set_code(0x25276, md::Code().jsr(injected_func).nop(2));
}

static void write_maps_entities(const World& world, md::ROM& rom)
{
    ByteArray entities_offsets_table, entities_table;

    for(auto& [map_id, map] : world.maps())
    {
        // Write map entities
        if(map->entities().empty())
        {
            entities_offsets_table.add_word(0x0000);
            continue;
        }

        entities_offsets_table.add_word(entities_table.size() + 1);

        for(Entity* entity : map->entities())
            entities_table.add_bytes(entity->to_bytes());
        entities_table.add_word(0xFFFF);
    }

    rom.mark_empty_chunk(offsets::MAP_ENTITIES_OFFSETS_TABLE, offsets::MAP_ENTITIES_OFFSETS_TABLE_END);
    rom.mark_empty_chunk(offsets::MAP_ENTITIES_TABLE, offsets::MAP_ENTITIES_TABLE_END);

    uint32_t entities_offsets_table_addr = rom.inject_bytes(entities_offsets_table);
    rom.set_code(0x1953A, md::Code().lea(entities_offsets_table_addr, reg_A0));

    uint32_t entities_table_addr = rom.inject_bytes(entities_table);
    rom.set_long(0x19566, entities_table_addr);
}

static void write_maps_entity_persistence_flags(const World& world, md::ROM& rom)
{
    uint32_t addr = offsets::PERSISTENCE_FLAGS_TABLE;
    std::vector<Entity*> sacred_trees_with_persistence;

    // Write switches persistence flags table
    for(auto& [map_id, map] : world.maps())
    {
        for(Entity* entity : map->entities())
        {
            if(entity->has_persistence_flag())
            {
                if(entity->entity_type_id() != ENTITY_SACRED_TREE)
                {
                    uint8_t flag_byte = entity->persistence_flag().byte & 0xFF;
                    uint8_t flag_bit = entity->persistence_flag().bit;
                    uint8_t entity_id = entity->entity_id();

                    rom.set_word(addr, entity->map()->id());
                    addr += 0x2;
                    rom.set_byte(addr, flag_byte);
                    addr += 0x1;
                    rom.set_byte(addr, (flag_bit << 5) | (entity_id & 0x1F));
                    addr += 0x1;
                }
                else
                {
                    sacred_trees_with_persistence.emplace_back(entity);
                }
            }
        }
    }

    rom.set_word(addr, 0xFFFF);
    addr += 0x2;
    if(addr > offsets::SACRED_TREES_PERSISTENCE_FLAGS_TABLE)
        throw LandstalkerException("Persistence flags table must not be bigger than the one from base game");
    rom.mark_empty_chunk(addr, offsets::SACRED_TREES_PERSISTENCE_FLAGS_TABLE);

    addr = offsets::SACRED_TREES_PERSISTENCE_FLAGS_TABLE;

    // Write sacred trees persistence flags table
    for(Entity* sacred_tree : sacred_trees_with_persistence)
    {
        uint8_t flag_byte = sacred_tree->persistence_flag().byte & 0xFF;
        uint8_t flag_bit = sacred_tree->persistence_flag().bit;

        rom.set_word(addr, sacred_tree->map()->id());
        addr += 0x2;
        rom.set_byte(addr, flag_byte);
        addr += 0x1;
        rom.set_byte(addr, flag_bit);
        addr += 0x1;
    }

    rom.set_word(addr, 0xFFFF);
    addr += 0x2;

    if(addr > offsets::SACRED_TREES_PERSISTENCE_FLAGS_TABLE_END)
        throw LandstalkerException("Sacred trees persistence flags table must not be bigger than the one from base game");
    rom.mark_empty_chunk(addr, offsets::SACRED_TREES_PERSISTENCE_FLAGS_TABLE_END);
}

static void write_maps(const World& world, md::ROM& rom, const std::map<MapLayout*, uint32_t>& map_layout_adresses)
{
    write_maps_data(world, rom, map_layout_adresses);
    write_maps_visited_flag(world, rom);
    write_maps_base_chest_id(world, rom);
    write_maps_climb_destination(world, rom);
    write_maps_fall_destination(world, rom);
    write_maps_variants(world, rom);
    write_maps_global_entity_masks(world, rom);
    write_maps_key_door_masks(world, rom);
    write_maps_entity_masks(world, rom);
    write_maps_dialogue_table(world, rom);
    write_maps_entities(world, rom);
    write_maps_entity_persistence_flags(world, rom);
}

///////////////////////////////////////////////////////////////////////////////

void io::write_world_to_rom(const World& world, md::ROM& rom)
{
    std::map<MapLayout*, uint32_t> map_layout_addresses = write_map_layouts(world, rom);
    write_blocksets(world, rom);
    write_item_names(world, rom);
    write_items(world, rom);
    write_item_sources(world, rom);
    write_entity_types(world, rom);
    write_entity_type_palettes(world, rom);
    write_game_strings(world, rom);
    write_fahl_enemies(world, rom);
    write_map_connections(world, rom);
    write_map_palettes(world, rom);
    write_maps(world, rom, map_layout_addresses);
}