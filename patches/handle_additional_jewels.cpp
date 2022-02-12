#include "../md_tools.hpp"

#include "../model/world.hpp"
#include "../model/map.hpp"
#include "../model/entity.hpp"
#include "../constants/item_codes.hpp"
#include "../constants/map_codes.hpp"
#include "../constants/offsets.hpp"
#include "../constants/values.hpp"
#include "../exceptions.hpp"
#include "../tools/sprite.hpp"

#include "assets/blue_jewel.bin.hxx"
#include "assets/yellow_jewel.bin.hxx"
#include "assets/green_jewel.bin.hxx"

static void add_jewel_check_for_kazalt_teleporter(md::ROM& rom, uint8_t jewel_count)
{
    // ----------- Rejection textbox handling ------------
    md::Code func_reject_kazalt_tp;

    func_reject_kazalt_tp.jsr(0x22EE8); // open textbox
    func_reject_kazalt_tp.movew(0x22, reg_D0);
    func_reject_kazalt_tp.jsr(0x28FD8); // display text 
    func_reject_kazalt_tp.jsr(0x22EA0); // close textbox
    func_reject_kazalt_tp.rts();

    uint32_t func_reject_kazalt_tp_addr = rom.inject_code(func_reject_kazalt_tp);

    // ----------- Jewel checks handling ------------
    md::Code proc_handle_jewels_check;

    if(jewel_count > MAX_INDIVIDUAL_JEWELS)
    {
        proc_handle_jewels_check.movem_to_stack({reg_D1},{});
        proc_handle_jewels_check.moveb(addr_(0xFF1054), reg_D1);
        proc_handle_jewels_check.andib(0x0F, reg_D1);
        proc_handle_jewels_check.cmpib(jewel_count, reg_D1); // Test if red jewel is owned
        proc_handle_jewels_check.movem_from_stack({reg_D1},{});
        proc_handle_jewels_check.ble("no_teleport");
    }
    else
    {
        if(jewel_count >= 5)
        {
            proc_handle_jewels_check.btst(0x1, addr_(0xFF1051)); // Test if yellow jewel is owned
            proc_handle_jewels_check.beq("no_teleport");
        }
        if(jewel_count >= 4)
        {
            proc_handle_jewels_check.btst(0x5, addr_(0xFF1050)); // Test if blue jewel is owned
            proc_handle_jewels_check.beq("no_teleport");
        }
        if(jewel_count >= 3)
        {
            proc_handle_jewels_check.btst(0x1, addr_(0xFF105A)); // Test if green jewel is owned
            proc_handle_jewels_check.beq("no_teleport");
        }
        if(jewel_count >= 2)
        {
            proc_handle_jewels_check.btst(0x1, addr_(0xFF1055)); // Test if purple jewel is owned
            proc_handle_jewels_check.beq("no_teleport");
        }
        if(jewel_count >= 1)
        {
            proc_handle_jewels_check.btst(0x1, addr_(0xFF1054)); // Test if red jewel is owned
            proc_handle_jewels_check.beq("no_teleport");
        }
    }

    // Teleport to Kazalt
    proc_handle_jewels_check.moveq(0x7, reg_D0);
    proc_handle_jewels_check.jsr(0xE110);  // "func_teleport_kazalt"
    proc_handle_jewels_check.jmp(0x62FA);

    // Rejection message
    proc_handle_jewels_check.label("no_teleport");
    proc_handle_jewels_check.jsr(func_reject_kazalt_tp_addr);
    proc_handle_jewels_check.rts();

    uint32_t handle_jewels_addr = rom.inject_code(proc_handle_jewels_check);

    // This adds the jewels as a requirement for the Kazalt teleporter to work correctly
    rom.set_code(0x62F4, md::Code().jmp(handle_jewels_addr));
}

static void remove_books_replaced_by_jewels(md::ROM& rom, World& world, uint8_t jewel_count)
{
    if(jewel_count > 3 && jewel_count <= MAX_INDIVIDUAL_JEWELS)
    {
        // Remove jewels replaced from book IDs from priest stands
        const std::vector<uint16_t> maps_to_clean = {
            MAP_MASSAN_CHURCH, MAP_GUMI_CHURCH, MAP_RYUMA_CHURCH, MAP_MERCATOR_CHURCH_VARIANT, MAP_VERLA_CHURCH,
            MAP_DESTEL_CHURCH, MAP_KAZALT_CHURCH
        };

        for(uint16_t map_id : maps_to_clean)
        {
            Map* map = world.map(map_id);
            for(int i = (int)map->entities().size()-1 ; i >= 0 ; --i)
            {
                uint8_t type_id = map->entities()[i]->entity_type_id();
                if(type_id == 0xC0 + ITEM_BLUE_JEWEL || type_id == 0xC0 + ITEM_YELLOW_JEWEL)
                    map->remove_entity(i);
            }
        }

        // Make the Awakening Book (the only one remaining in churches) heal all status conditions
        rom.set_code(0x24F6C, md::Code().nop(6));
        rom.set_code(0x24FB8, md::Code().moveb(0xFF, reg_D0));

        // Change the behavior of AntiCurse and Detox books (now Yellow and Blue jewels) in shops
        rom.set_byte(0x24C40, 0x40);
        rom.set_byte(0x24C58, 0x40);
    }
}

/**
 * The randomizer has an option to handle more jewels than the good old Red and Purple ones.
 * It can handle up to 9 jewels, but we only can afford having 5 unique jewel items.
 * This means there are two modes:
 *      - Unique jewels mode (1-5 jewels): each jewel has its own ID, name and sprite
 *      - Kazalt jewels mode (6+ jewels): jewels are one generic item that can be stacked up to 9 times
 *
 * This function handles the "unique jewels mode" by replacing useless items (priest books), injecting
 * new sprites and taking care of everything for this to happen.
 */
static void add_additional_jewel_sprites(md::ROM& rom, uint8_t jewel_count)
{
    // If we are in "Kazalt jewel" mode, don't do anything
    if(jewel_count > MAX_INDIVIDUAL_JEWELS)
        return;

    SubSpriteMetadata subsprite(0x77FB);

    if(jewel_count >= 3)
    {
        // Add a sprite for green jewel and make the item use it
        Sprite green_jewel_sprite(GREEN_JEWEL_SPRITE, GREEN_JEWEL_SPRITE_SIZE, { subsprite });
        uint32_t green_jewel_sprite_addr = rom.inject_bytes(green_jewel_sprite.encode());
        rom.set_long(offsets::ITEM_SPRITES_TABLE + (ITEM_GREEN_JEWEL * 0x4), green_jewel_sprite_addr); // 0x121648
    }
    if(jewel_count >= 4)
    {
        // Add a sprite for blue jewel and make the item use it
        Sprite blue_jewel_sprite(BLUE_JEWEL_SPRITE, BLUE_JEWEL_SPRITE_SIZE, { subsprite });
        uint32_t blue_jewel_sprite_addr = rom.inject_bytes(blue_jewel_sprite.encode());
        rom.set_long(offsets::ITEM_SPRITES_TABLE + (ITEM_BLUE_JEWEL * 0x4), blue_jewel_sprite_addr);
    }
    if(jewel_count >= 5)
    {
        // Add a sprite for green jewel and make the item use it
        Sprite yellow_jewel_sprite(YELLOW_JEWEL_SPRITE, YELLOW_JEWEL_SPRITE_SIZE, { subsprite });
        uint32_t yellow_jewel_sprite_addr = rom.inject_bytes(yellow_jewel_sprite.encode());
        rom.set_long(offsets::ITEM_SPRITES_TABLE + (ITEM_YELLOW_JEWEL * 0x4), yellow_jewel_sprite_addr);
    }
}

void handle_additional_jewels(md::ROM& rom, World& world, uint8_t jewel_count)
{
    add_jewel_check_for_kazalt_teleporter(rom, jewel_count);
    remove_books_replaced_by_jewels(rom, world, jewel_count);
    add_additional_jewel_sprites(rom, jewel_count);
}
