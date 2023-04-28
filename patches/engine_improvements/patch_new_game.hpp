#pragma once

#include "../game_patch.hpp"
#include "../../model/world.hpp"
#include "../../model/item.hpp"
#include "../../constants/item_codes.hpp"

/**
 * This patch edits the behavior when selecting "New Game", changing the way the game is initialized.
 * For instance, it setups the spawn location with values stored inside the World model
 */
class PatchNewGame : public GamePatch
{
private:
    bool _add_ingame_tracker = false;

public:
    explicit PatchNewGame(bool add_ingame_tracker) : _add_ingame_tracker(add_ingame_tracker) {}

    void alter_rom(md::ROM& rom) override
    {
        // Usually, when starting a new game, it is automatically put into "cutscene mode" to
        // let the intro roll without allowing the player to move or pause, or do anything at all.
        // We need to remove that cutscene flag to enable the player actually playing the game.
        rom.set_code(0x281A, md::Code().nop(4));

        // Loading a save inside the map having the intro ID triggers the intro. That's not the kind
        // of behavior we want.
        rom.set_code(0x8DDA, md::Code().nop(5));

        // Load HUD tiles properly when loading inside intro map
        rom.set_code(0x8F6E, md::Code().nop(4));
        rom.set_byte(0x8F76, 0x60);
    }

    void inject_code(md::ROM& rom, World& world) override
    {
        // Edit spawn map and position
        rom.set_word(0x0027F4, world.spawn_map_id());
        rom.set_byte(0x0027FD, world.spawn_position_x());
        rom.set_byte(0x002805, world.spawn_position_y());

        // Edit starting life
        uint8_t starting_life = world.starting_life();
        rom.set_byte(0x0027B4, starting_life-1);
        rom.set_byte(0x0027BC, starting_life-1);

        // Inject a function initializing everyhting properly on new game (flags...)
        uint32_t func_init_game = inject_func_init_game(rom, world);
        rom.set_code(0x002700, md::Code().jsr(func_init_game).nop());
        // Replace the bitset of the "no music flag" by a jump to 'func_init_game'
        // 0x002700:
        // Before: 	[08F9] bset 3 -> $FF1027
        // After:	[4EB9] jsr func_init_game ; [4E71] nop
    }

private:
    [[nodiscard]] std::vector<uint8_t> build_flag_array(const World& world) const
    {
        std::vector<uint8_t> flag_array;
        flag_array.resize(0x80, 0x00);

        // Apply starting flags
        for(const Flag& flag : world.starting_flags())
            flag_array[(uint8_t)flag.byte] |= (1 << flag.bit);

        // Apply starting inventory
        for(uint8_t item_id=0 ; item_id < ITEM_COUNT ; item_id += 0x2)
        {
            uint8_t inventory_flag_value = 0x00;

            uint8_t lsh_quantity = world.item(item_id)->starting_quantity();
            if(lsh_quantity)
                inventory_flag_value |= (lsh_quantity+1) & 0x0F;

            uint8_t msh_quantity = world.item(item_id+1)->starting_quantity();
            if(msh_quantity)
                inventory_flag_value |= ((msh_quantity+1) & 0x0F) << 4;

            flag_array[0x40 + (item_id / 2)] = inventory_flag_value;
        }

        if(_add_ingame_tracker)
        {
            // The ingame-tracker consists in putting in "grayed-out" key items in the inventory,
            // as if they were already obtained but lost (like lithograph in OG)
            flag_array[0x4B] |= 0x10;
            flag_array[0x4C] |= 0x10;
            flag_array[0x4D] |= 0x11;
            flag_array[0x4F] |= 0x11;
            flag_array[0x50] |= 0x01;
            flag_array[0x55] |= 0x10;
            flag_array[0x57] |= 0x10;
            flag_array[0x58] |= 0x10;
            flag_array[0x59] |= 0x11;
            flag_array[0x5B] |= 0x10;
            flag_array[0x5C] |= 0x11;
        }

        return flag_array;
    }

    [[nodiscard]] uint32_t inject_func_init_game(md::ROM& rom, const World& world) const
    {
        std::vector<uint8_t> flag_array = build_flag_array(world);

        md::Code func_init_game;
        {
            // Init all flags with the contents of flag_array
            for(int i = 0 ; i < flag_array.size() ; i += 0x2)
            {
                uint16_t value = (static_cast<uint16_t>(flag_array[i]) << 8) + static_cast<uint16_t>(flag_array[i + 1]);
                if(value)
                    func_init_game.movew(value, addr_(0xFF1000 + i));
            }

            // Set the "parent map" value to the same value as "current map" so that dialogues work on spawn map
            func_init_game.movew(world.spawn_map_id(), addr_(0xFF1206));
            // Set the orientation byte of Nigel depending on spawn location on game start
            func_init_game.moveb(world.spawn_orientation(), addr_(0xFF5404));
            // Set the appropriate starting golds
            func_init_game.movew(world.starting_golds(), addr_(0xFF120E));
        }
        func_init_game.rts();

        return rom.inject_code(func_init_game);
    }
};

