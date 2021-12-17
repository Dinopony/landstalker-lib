#include "../md_tools.hpp"
#include "../constants/offsets.hpp"
#include "../tools/sprite.hpp"

constexpr uint8_t COLOR_SWORD_WRONG = 9;
constexpr uint8_t COLOR_SWORD_REPLACEMENT = 6;

static void set_nigel_color(md::ROM& rom, uint8_t color_id, uint16_t color)
{
    uint32_t color_address = offsets::NIGEL_PALETTE + (color_id * 2);
    rom.set_word(color_address, color);

    color_address = offsets::NIGEL_PALETTE_SAVE_MENU + (color_id * 2);
    rom.set_word(color_address, color);
}

static void fix_sword_shade_frame(md::ROM& rom, uint32_t address, const std::vector<uint8_t>& tile_ids)
{
    Sprite nigel_frame = Sprite::decode_from(rom.iterator_at(address));
    for(uint8_t tile_id : tile_ids)
        nigel_frame.replace_color_in_tile(COLOR_SWORD_WRONG, COLOR_SWORD_REPLACEMENT, tile_id);
    rom.set_bytes(address, nigel_frame.encode());
}

static void fix_sword_shade_frame(md::ROM& rom, uint32_t address, const uint8_t tile_id)
{
    fix_sword_shade_frame(rom, address, std::vector<uint8_t>({tile_id}));
}

static void fix_sword_shade(md::ROM& rom)
{
    fix_sword_shade_frame(rom, 0x1220AE, 11); // SpriteGfx000Frame02
    fix_sword_shade_frame(rom, 0x122A7C, 11); // SpriteGfx000Frame06
    fix_sword_shade_frame(rom, 0x122D1A, 11); // SpriteGfx000Frame07
    fix_sword_shade_frame(rom, 0x122F62, 11); // SpriteGfx000Frame08
    fix_sword_shade_frame(rom, 0x1231A0, 11); // SpriteGfx000Frame09

    Sprite nigel_frame_10 = Sprite::decode_from(rom.iterator_at(0x1233A8));
    nigel_frame_10.replace_color_in_tile(COLOR_SWORD_WRONG, COLOR_SWORD_REPLACEMENT, 11);
    nigel_frame_10.set_pixel(10, 7, 6, COLOR_SWORD_REPLACEMENT);
    nigel_frame_10.set_pixel(10, 6, 7, COLOR_SWORD_REPLACEMENT);
    rom.set_bytes(0x1233A8, nigel_frame_10.encode());

    fix_sword_shade_frame(rom, 0x123626, 11); // SpriteGfx000Frame11
    fix_sword_shade_frame(rom, 0x123FA8, 11); // SpriteGfx000Frame15

    Sprite nigel_frame_16 = Sprite::decode_from(rom.iterator_at(0x124200));
    nigel_frame_16.replace_color_in_tile(COLOR_SWORD_WRONG, COLOR_SWORD_REPLACEMENT, 11);
    nigel_frame_16.set_pixel(11, 0, 2, COLOR_SWORD_WRONG);
    rom.set_bytes(0x124200, nigel_frame_16.encode());

    Sprite nigel_frame_18 = Sprite::decode_from(rom.iterator_at(0x124694));
    nigel_frame_18.replace_color_in_tile(COLOR_SWORD_WRONG, COLOR_SWORD_REPLACEMENT, 6);
    nigel_frame_18.set_pixel(10, 3, 7, COLOR_SWORD_REPLACEMENT);
    nigel_frame_18.replace_color_in_tile(COLOR_SWORD_WRONG, COLOR_SWORD_REPLACEMENT, 11);
    rom.set_bytes(0x124694, nigel_frame_18.encode());

    fix_sword_shade_frame(rom, 0x12493C, 11); // SpriteGfx000Frame19
    fix_sword_shade_frame(rom, 0x124BE2, 11); // SpriteGfx000Frame20

    // --- Anim dropping held item, facing camera ----------------------------
    fix_sword_shade_frame(rom, 0x12510A, 15); // SpriteGfx000Frame22

    Sprite nigel_frame_23 = Sprite::decode_from(rom.iterator_at(0x125370));
    nigel_frame_23.set_pixel(15, 5, 0, COLOR_SWORD_REPLACEMENT);
    nigel_frame_23.set_pixel(15, 5, 1, COLOR_SWORD_REPLACEMENT);
    nigel_frame_23.set_pixel(15, 6, 1, COLOR_SWORD_REPLACEMENT);
    nigel_frame_23.set_pixel(15, 6, 2, COLOR_SWORD_REPLACEMENT);
    rom.set_bytes(0x125370, nigel_frame_23.encode());

    // --- Anim held item, not looking camera ----------------------------
    fix_sword_shade_frame(rom, 0x125616, 11); // SpriteGfx000Frame24
    fix_sword_shade_frame(rom, 0x1258C2, {10, 11}); // SpriteGfx000Frame25
    fix_sword_shade_frame(rom, 0x125B70, 11); // SpriteGfx000Frame26
    fix_sword_shade_frame(rom, 0x125E24, 11); // SpriteGfx000Frame27
    fix_sword_shade_frame(rom, 0x1260DA, 11); // SpriteGfx000Frame28
    fix_sword_shade_frame(rom, 0x126374, 11); // SpriteGfx000Frame29
    fix_sword_shade_frame(rom, 0x1265E8, 11); // SpriteGfx000Frame30
    fix_sword_shade_frame(rom, 0x126866, 11); // SpriteGfx000Frame31

    // --- Anim held item, facing camera ----------------------------
    Sprite nigel_frame_32 = Sprite::decode_from(rom.iterator_at(0x126AC4));
    nigel_frame_32.set_pixel(10, 6, 7, COLOR_SWORD_REPLACEMENT);
    nigel_frame_32.set_pixel(11, 5, 0, COLOR_SWORD_REPLACEMENT);
    nigel_frame_32.set_pixel(11, 4, 1, COLOR_SWORD_REPLACEMENT);
    rom.set_bytes(0x126AC4, nigel_frame_32.encode());

    Sprite nigel_frame_33 = Sprite::decode_from(rom.iterator_at(0x126D66));
    nigel_frame_33.set_pixel(10, 6, 7, COLOR_SWORD_REPLACEMENT);
    nigel_frame_33.set_pixel(11, 5, 0, COLOR_SWORD_REPLACEMENT);
    rom.set_bytes(0x126D66, nigel_frame_33.encode());

    fix_sword_shade_frame(rom, 0x12700C, 11); // SpriteGfx000Frame34

    Sprite nigel_frame_35 = Sprite::decode_from(rom.iterator_at(0x1272AC));
    nigel_frame_35.set_pixel(10, 6, 7, COLOR_SWORD_REPLACEMENT);
    nigel_frame_35.replace_color_in_tile(COLOR_SWORD_WRONG, COLOR_SWORD_REPLACEMENT, 11);
    rom.set_bytes(0x1272AC, nigel_frame_35.encode());

    Sprite nigel_frame_36 = Sprite::decode_from(rom.iterator_at(0x127546));
    nigel_frame_36.set_pixel(10, 6, 7, COLOR_SWORD_REPLACEMENT);
    nigel_frame_36.set_pixel(11, 5, 0, COLOR_SWORD_REPLACEMENT);
    nigel_frame_36.set_pixel(11, 4, 1, COLOR_SWORD_REPLACEMENT);
    rom.set_bytes(0x127546, nigel_frame_36.encode());

    Sprite nigel_frame_37 = Sprite::decode_from(rom.iterator_at(0x1277D0));
    nigel_frame_37.set_pixel(10, 5, 7, COLOR_SWORD_REPLACEMENT);
    nigel_frame_37.set_pixel(10, 6, 6, COLOR_SWORD_REPLACEMENT);
    nigel_frame_37.set_pixel(11, 4, 0, COLOR_SWORD_REPLACEMENT);
    rom.set_bytes(0x1277D0, nigel_frame_37.encode());

    fix_sword_shade_frame(rom, 0x127A50, 11); // SpriteGfx000Frame38

    Sprite nigel_frame_39 = Sprite::decode_from(rom.iterator_at(0x127CC8));
    nigel_frame_39.set_pixel(11, 6, 1, COLOR_SWORD_REPLACEMENT);
    nigel_frame_39.set_pixel(11, 5, 2, COLOR_SWORD_REPLACEMENT);
    nigel_frame_39.set_pixel(11, 4, 3, COLOR_SWORD_REPLACEMENT);
    rom.set_bytes(0x127CC8, nigel_frame_39.encode());

    // --- Anim jumping, not looking at camera ----------------------------
    fix_sword_shade_frame(rom, 0x127F28, 10); // SpriteGfx000Frame40
    fix_sword_shade_frame(rom, 0x128126, 10); // SpriteGfx000Frame41

    // --- Anim jumping, facing camera ----------------------------
    fix_sword_shade_frame(rom, 0x128340, 10); // SpriteGfx000Frame42
    fix_sword_shade_frame(rom, 0x12859A, {10, 11}); // SpriteGfx000Frame43

    // --- Anim jumping holding something, not looking at camera ----------------------------
    fix_sword_shade_frame(rom, 0x1287FA, 10); // SpriteGfx000Frame44
    fix_sword_shade_frame(rom, 0x128A10, 10); // SpriteGfx000Frame45

    // --- Anim jumping holding something, facing camera ----------------------------
    fix_sword_shade_frame(rom, 0x128C36, 10); // SpriteGfx000Frame46
    fix_sword_shade_frame(rom, 0x128E6C, {10, 11}); // SpriteGfx000Frame47

    // --- Anim jumping throwing something, not looking at camera ----------------------------
    fix_sword_shade_frame(rom, 0x1290C4, {10, 14, 15}); // SpriteGfx000Frame48
    fix_sword_shade_frame(rom, 0x129324, 10); // SpriteGfx000Frame49

    // --- Anim jumping throwing something, facing camera ----------------------------
    fix_sword_shade_frame(rom, 0x1295D2, {6, 10, 11}); // SpriteGfx000Frame50

    // --- Anim climbing, not looking at camera ----------------------------
    Sprite nigel_frame_60 = Sprite::decode_from(rom.iterator_at(0x12B990));
    nigel_frame_60.set_pixel(10, 1, 7, COLOR_SWORD_REPLACEMENT);
    nigel_frame_60.set_pixel(10, 5, 7, COLOR_SWORD_REPLACEMENT);
    nigel_frame_60.set_pixel(10, 6, 7, COLOR_SWORD_REPLACEMENT);
    nigel_frame_60.replace_color_in_tile(COLOR_SWORD_WRONG, COLOR_SWORD_REPLACEMENT, 11);
    rom.set_bytes(0x12B990, nigel_frame_60.encode());

    fix_sword_shade_frame(rom, 0x12BB9E, {10, 11}); // SpriteGfx000Frame61

    Sprite nigel_frame_62 = Sprite::decode_from(rom.iterator_at(0x12BDFE));
    nigel_frame_62.set_pixel(10, 2, 7, COLOR_SWORD_REPLACEMENT);
    nigel_frame_62.replace_color_in_tile(COLOR_SWORD_WRONG, COLOR_SWORD_REPLACEMENT, 11);
    rom.set_bytes(0x12BDFE, nigel_frame_62.encode());
}

/// colors follows the (0BGR) format used by palettes
void alter_nigel_colors(md::ROM& rom, const std::pair<uint16_t, uint16_t>& nigel_colors)
{
    set_nigel_color(rom, 9, nigel_colors.first);    // Light color
    set_nigel_color(rom, 10, nigel_colors.second);  // Dark color

    fix_sword_shade(rom);
}
