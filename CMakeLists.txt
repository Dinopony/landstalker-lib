cmake_minimum_required(VERSION 3.21.0)

project(landstalker-lib VERSION 1.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif ()

add_compile_definitions(RELEASE="${PROJECT_VERSION}")
add_compile_definitions(MAJOR_RELEASE="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

macro(WRAPPED_DEPENDENCY)
    message(STATUS "Defining wrapped dependency for ${CMAKE_CURRENT_SOURCE_DIR}/${ARGV1}")
    add_custom_command(
            OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${ARGV1}.hxx"
            COMMAND "${CMAKE_COMMAND}"
            ARGS -DINPUT_FILE="${ARGV1}" -DVARIABLE_NAME="${ARGV2}" -DPROCESS_TYPE="${ARGV0}" -P wrap_file.cmake
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${ARGV1}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endmacro()

# Wrap JSON model files and assets into automated .hxx headers
wrapped_dependency(TEXT     model/data/entity_type.json             ENTITIES_JSON)
wrapped_dependency(TEXT     model/data/item_source.json             ITEM_SOURCES_JSON)
wrapped_dependency(RAW      patches/assets/fixed_hud_tilemap.bin    FIXED_HUD_TILEMAP)
wrapped_dependency(RAW      patches/assets/blue_jewel.bin           BLUE_JEWEL_SPRITE)
wrapped_dependency(RAW      patches/assets/green_jewel.bin          GREEN_JEWEL_SPRITE)
wrapped_dependency(RAW      patches/assets/yellow_jewel.bin         YELLOW_JEWEL_SPRITE)

set(SOURCES
        # --- Auto-generated dependencies----------------------------------------
        "model/data/entity_type.json.hxx"
        "model/data/item_source.json.hxx"
        "patches/assets/fixed_hud_tilemap.bin.hxx"
        "patches/assets/blue_jewel.bin.hxx"
        "patches/assets/green_jewel.bin.hxx"
        "patches/assets/yellow_jewel.bin.hxx"

        # --- Global includes ----------------------------------------
        "md_tools.hpp"
        "exceptions.hpp"

        # --- Megadrive tools ----------------------------------------
        "md_tools/code.hpp"
        "md_tools/code.cpp"
        "md_tools/rom.hpp"
        "md_tools/rom.cpp"
        "md_tools/types.hpp"

        # --- Constants ----------------------------------------
        "constants/entity_type_codes.hpp"
        "constants/flags.hpp"
        "constants/item_codes.hpp"
        "constants/map_codes.hpp"
        "constants/offsets.hpp"
        "constants/symbols.hpp"
        "constants/values.hpp"

        # --- I/O ----------------------------------------
        "io/io.hpp"
        "io/blocksets_decoder.cpp"
        "io/blocksets_encoder.cpp"
        "io/exports.cpp"
        "io/map_layout_decoder.cpp"
        "io/map_layout_encoder.cpp"
        "io/world_rom_reader.cpp"
        "io/world_rom_writer.cpp"
        "io/textbanks_decoder.cpp"
        "io/textbanks_encoder.cpp"

        # --- Tools ----------------------------------------
        "tools/argument_dictionary.hpp"
        "tools/bitstream_reader.hpp"
        "tools/bitstream_reader.cpp"
        "tools/bitstream_writer.hpp"
        "tools/byte_array.hpp"
        "tools/flag.hpp"
        "tools/huffman_tree.hpp"
        "tools/huffman_tree_node.hpp"
        "tools/json.hpp"
        "tools/color.hpp"
        "tools/color.cpp"
        "tools/color_palette.hpp"
        "tools/game_text.hpp"
        "tools/game_text.cpp"
        "tools/lz77.hpp"
        "tools/lz77.cpp"
        "tools/sprite.hpp"
        "tools/sprite.cpp"
        "tools/tile_queue.hpp"
        "tools/stringtools.hpp"
        "tools/vectools.hpp"

        # --- Model ----------------------------------------
        "model/blockset.hpp"
        "model/entity_type.hpp"
        "model/entity_type.cpp"
        "model/entity.hpp"
        "model/entity.cpp"
        "model/item_source.hpp"
        "model/item_source.cpp"
        "model/item.hpp"
        "model/item.cpp"
        "model/map_connection.hpp"
        "model/map_layout.hpp"
        "model/map.hpp"
        "model/map.cpp"
        "model/spawn_location.hpp"
        "model/tile.hpp"
        "model/world.hpp"
        "model/world.cpp"

        # --- Patches ----------------------------------------
        "patches/add_function_to_items_on_use.cpp"
        "patches/add_statue_of_jypta_effect.cpp"
        "patches/alter_fahl_challenge.cpp"
        "patches/alter_gold_rewards_handling.cpp"
        "patches/alter_items_consumability.cpp"
        "patches/alter_lantern_handling.cpp"
        "patches/alter_nigel_colors.cpp"
        "patches/alter_ui_color.cpp"
        "patches/fix_hud_tilemap.cpp"
        "patches/fix_item_checks.cpp"
        "patches/game_init.cpp"
        "patches/gameplay_tweaks.cpp"
        "patches/handle_additional_jewels.cpp"
        "patches/handle_armor_upgrades.cpp"
        "patches/handle_items_renaming.cpp"
        "patches/improve_engine.cpp"
        "patches/make_sword_of_gaia_work_in_volcano.cpp"
        "patches/normalize_special_enemies_hp.cpp"
        "patches/quality_of_life.cpp"
        "patches/story_dependencies.cpp"
        )

add_library(landstalker_lib STATIC "${SOURCES}")
