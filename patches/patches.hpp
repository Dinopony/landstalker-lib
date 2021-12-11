#pragma once

#include <landstalker_lib/md_tools.hpp>
#include <landstalker_lib/model/world.hpp>

// Global patches
void add_functions_to_items_on_use(md::ROM& rom, const World& world, bool consumable_record_book);
void add_statue_of_jypta_effect(md::ROM& rom);
void alter_fahl_challenge(md::ROM& rom, const World& world);
void alter_gold_rewards_handling(md::ROM& rom, World& world);
void alter_lantern_handling(md::ROM& rom, const World& world);
void alter_nigel_colors(md::ROM& rom, const std::pair<uint16_t, uint16_t>& nigel_colors);
void alter_ui_color(md::ROM& rom, uint16_t ui_color);
void fix_hud_tilemap(md::ROM& rom);
void fix_item_checks(md::ROM& rom);
void patch_game_init(md::ROM& rom, const World& world, bool add_ingame_tracker);
void handle_additional_jewels(md::ROM& rom, World& world, uint8_t jewel_count);
void handle_armor_upgrades(md::ROM& rom);
void make_sword_of_gaia_work_in_volcano(md::ROM& rom);
void normalize_special_enemies_hp(md::ROM& rom, bool fix_tree_cutting_glitch);

// alter_items_consumability.cpp patches
void make_pawn_ticket_consumable(md::ROM& rom);
void make_key_not_consumed_on_use(md::ROM& rom);

// quality_of_life.cpp patches
void alter_item_order_in_menu(md::ROM& rom);
void quicken_gaia_effect(md::ROM& rom);

// story_dependencies.cpp patches
void make_massan_elder_reward_not_story_dependant(md::ROM& rom);
void make_lumberjack_reward_not_story_dependant(md::ROM& rom);
void change_falling_ribbon_position(md::ROM& rom);
void make_tibor_always_open(md::ROM& rom);
void make_gumi_boulder_push_not_story_dependant(World& world);
void make_falling_ribbon_not_story_dependant(World& world);
