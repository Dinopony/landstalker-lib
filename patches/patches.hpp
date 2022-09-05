#pragma once

#include <landstalker_lib/md_tools.hpp>
#include <landstalker_lib/model/world.hpp>
#include "../tools/color.hpp"

#include "cosmetic/patch_alter_inventory_order.hpp"
#include "cosmetic/patch_alter_nigel_colors.hpp"
#include "cosmetic/patch_alter_ui_color.hpp"
#include "cosmetic/patch_fix_hud_tilemap.hpp"

#include "engine_improvements/patch_add_soft_reset.hpp"
#include "engine_improvements/patch_disable_region_check.hpp"
#include "engine_improvements/patch_extend_rom.hpp"
#include "engine_improvements/patch_extend_tile_swap_flag_range.hpp"
#include "engine_improvements/patch_extend_visited_flag_range.hpp"
#include "engine_improvements/patch_faster_transitions.hpp"
#include "engine_improvements/patch_improve_ingame_timer.hpp"
#include "engine_improvements/patch_improve_item_use_handling.hpp"
#include "engine_improvements/patch_new_game.hpp"
#include "engine_improvements/patch_new_map_format.hpp"
#include "engine_improvements/patch_normalize_bosses_hp.hpp"
#include "engine_improvements/patch_optimize_ai_lookup.hpp"
#include "engine_improvements/patch_optimize_reorder_draw_order_list.hpp"
#include "engine_improvements/patch_optimize_collision_detect.hpp"

void execute_patches(const std::vector<GamePatch*>& patches, md::ROM& rom, World& world)
{
    for(GamePatch* patch : patches) patch->load_from_rom(rom);
    for(GamePatch* patch : patches) patch->alter_rom(rom);
    for(GamePatch* patch : patches) patch->alter_world(world);
    for(GamePatch* patch : patches) patch->clear_space_in_rom(rom);
    for(GamePatch* patch : patches) patch->inject_data(rom, world);
    for(GamePatch* patch : patches) patch->inject_code(rom, world);
    for(GamePatch* patch : patches) patch->postprocess(rom, world);
    for(GamePatch* patch : patches) delete patch;
}