cmake_minimum_required(VERSION 3.1.0)

include("CMakeFunctions.txt")

project(landstalker-lib VERSION 1.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif ()

# Wrap JSON model files into automated .hxx headers
wrap_file(SOURCE_FILE "model/data/entity_type.json" VARIABLE_NAME "ENTITIES_JSON" TEXT_FILE)
wrap_file(SOURCE_FILE "model/data/item.json" VARIABLE_NAME "ITEMS_JSON" TEXT_FILE)
wrap_file(SOURCE_FILE "model/data/item_source.json" VARIABLE_NAME "ITEM_SOURCES_JSON" TEXT_FILE)
wrap_file(SOURCE_FILE "model/data/world_teleport_tree.json" VARIABLE_NAME "WORLD_TELEPORT_TREES_JSON" TEXT_FILE)

# Wrap assets files into automated .hxx headers
wrap_file(SOURCE_FILE "patches/assets/fixed_hud_tilemap.bin" VARIABLE_NAME "FIXED_HUD_TILEMAP")
wrap_file(SOURCE_FILE "patches/assets/blue_jewel.bin" VARIABLE_NAME "BLUE_JEWEL_SPRITE" SPRITE_FILE)
wrap_file(SOURCE_FILE "patches/assets/green_jewel.bin" VARIABLE_NAME "GREEN_JEWEL_SPRITE" SPRITE_FILE)
wrap_file(SOURCE_FILE "patches/assets/yellow_jewel.bin" VARIABLE_NAME "YELLOW_JEWEL_SPRITE" SPRITE_FILE)

add_compile_definitions(RELEASE="${PROJECT_VERSION}")
add_compile_definitions(MAJOR_RELEASE="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

set(SOURCES
        # --- Global includes ----------------------------------------
        "md_tools.hpp"
        "exceptions.hpp"

        # --- Megadrive tools ----------------------------------------
        "md_tools/code.hpp"
        "md_tools/rom.hpp"
        "md_tools/types.hpp"

        "md_tools/code.cpp"
        "md_tools/rom.cpp"

        # --- Constants ----------------------------------------
        "constants/entity_type_codes.hpp"
        "constants/flags.hpp"
        "constants/item_codes.hpp"
        "constants/map_codes.hpp"
        "constants/offsets.hpp"
        "constants/values.hpp"

        # --- Tools ----------------------------------------
        "tools/argument_dictionary.hpp"
        "tools/bitfield.hpp"
        "tools/byte_array.hpp"
        "tools/flag.hpp"
        "tools/unsorted_set.hpp"
        "tools/tools.hpp"
        "tools/json.hpp"

        "tools/game_text.hpp"
        "tools/game_text.cpp"
        "tools/textbanks_decoder.hpp"
        "tools/textbanks_decoder.cpp"
        "tools/textbanks_encoder.hpp"
        "tools/textbanks_encoder.cpp"

        "tools/huffman/tree.hpp"
        "tools/huffman/tree_node.hpp"
        "tools/huffman/symbols.hpp"

        # --- Model ----------------------------------------
        "model/entity_type.hpp"
        "model/entity_type.cpp"
        "model/entity.hpp"
        "model/entity.cpp"
        "model/item_source.hpp"
        "model/item_source.cpp"
        "model/item.hpp"
        "model/item.cpp"
        "model/map_connection.hpp"
        "model/map_palette.hpp"
        "model/map_palette.cpp"
        "model/map.hpp"
        "model/map.cpp"
        "model/spawn_location.hpp"
        "model/world_teleport_tree.hpp"
        "model/world.hpp"
        "model/world.cpp"

        # --- Patches ----------------------------------------
        "patches/add_function_to_items_on_use.cpp"
        "patches/add_statue_of_jypta_effect.cpp"
        "patches/alter_fahl_challenge.cpp"
        "patches/alter_gold_rewards_handling.cpp"
        "patches/alter_items_consumability.cpp"
        "patches/alter_lantern_handling.cpp"
        "patches/alter_ui_color.cpp"
        "patches/fix_hud_tilemap.cpp"
        "patches/fix_item_checks.cpp"
        "patches/game_init.cpp"
        "patches/handle_additional_jewels.cpp"
        "patches/make_sword_of_gaia_work_in_volcano.cpp"
        "patches/normalize_special_enemies_hp.cpp"
        "patches/quality_of_life.cpp"
        "patches/story_dependencies.cpp"

        # --- I/O ----------------------------------------
        "io/world_rom_reader.hpp"
        "io/world_rom_reader.cpp"
        "io/world_rom_writer.hpp"
        "io/world_rom_writer.cpp"
        )

add_library(landstalker_lib STATIC "${SOURCES}")
