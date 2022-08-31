#pragma once

#include "../game_patch.hpp"
#include "../../model/map.hpp"

/**
 * In the base game, the visited flags lookup is indexed on FF10C0, which means we can only set a flag above
 * FF10C0 when entering a room.
 * This patch changes this so visited flags are now indexed on FF1000, making any flag change possible.
 */
class PatchExtendVisitedFlagRange : public GamePatch
{
public:
    void alter_rom(md::ROM& rom) override
    {
        // Change the function to index the table off FF1000 instead of FF10C0
        rom.set_long(0x2954, 0xFF1000);
    }

    void alter_world(World& world) override
    {
        // Increment all visited flag bytes by 0xC0 to adapt to the new system
        for(auto& [map_id, map] : world.maps())
        {
            Flag visited_flag = map->visited_flag();
            visited_flag.byte += 0xC0;
            map->visited_flag(visited_flag);
        }
    }
};
