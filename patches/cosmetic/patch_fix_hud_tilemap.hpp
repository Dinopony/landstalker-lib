#pragma once

#include "../game_patch.hpp"
#include "../../constants/offsets.hpp"
#include "../../assets/fixed_hud_tilemap.bin.hxx"

/**
 * In the original game, the HUD tilemap has a problem on the zero character not having a shadow.
 * This is fixed in this edited version of the HUD tilemap.
 */
class PatchFixHUDTilemap : public GamePatch
{
public:
    void alter_rom(md::ROM& rom) override
    {
        rom.set_bytes(offsets::HUD_TILEMAP, FIXED_HUD_TILEMAP, FIXED_HUD_TILEMAP_SIZE);
    }
};

