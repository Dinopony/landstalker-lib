cmake_minimum_required(VERSION 3.21.0)

project(landstalker-lib VERSION 1.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif ()

add_compile_definitions(RELEASE="${PROJECT_VERSION}")
add_compile_definitions(MAJOR_RELEASE="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

if(DEBUG)
    add_compile_definitions(DEBUG)
endif()

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
wrapped_dependency(TEXT     assets/entity_type_names.json           ENTITY_TYPE_NAMES_JSON)
wrapped_dependency(RAW      assets/fixed_hud_tilemap.bin            FIXED_HUD_TILEMAP)

set(SOURCES
        # --- Auto-generated dependencies----------------------------------------
        "assets/entity_type_names.json.hxx"
        "assets/fixed_hud_tilemap.bin.hxx"

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
        "io/map_layout_new_encoder.cpp"
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
        "tools/lodepng.h"
        "tools/lodepng.cpp"

        # --- Model ----------------------------------------
        "model/blockset.hpp"
        "model/entity_type.hpp"
        "model/entity_type.cpp"
        "model/entity.hpp"
        "model/entity.cpp"
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
        "patches/engine_improvements/patch_add_soft_reset.hpp"
        "patches/engine_improvements/patch_extend_visited_flag_range.hpp"
        "patches/engine_improvements/patch_extend_tile_swap_flag_range.hpp"
        "patches/engine_improvements/patch_improve_ingame_timer.hpp"
        "patches/engine_improvements/patch_improve_item_use_handling.hpp"
        "patches/engine_improvements/patch_new_map_format.hpp"
        "patches/engine_improvements/patch_normalize_bosses_hp.hpp"
        "patches/engine_improvements/patch_faster_transitions.hpp"
        "patches/engine_improvements/patch_disable_region_check.hpp"
        "patches/engine_improvements/patch_new_game.hpp"

        "patches/cosmetic/patch_alter_nigel_colors.hpp"
        "patches/cosmetic/patch_alter_ui_color.hpp"
        "patches/cosmetic/patch_alter_inventory_order.hpp"

        "patches/game_patch.hpp"
        )

add_library(landstalker_lib STATIC "${SOURCES}")
