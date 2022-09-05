#pragma once

#include "../game_patch.hpp"
#include "../../constants/offsets.hpp"

/**
 * In the vanilla engine, the game performs a lookup for EVERY ENTITY on EVERY FRAME to get its AI function address.
 * This lookup is very CPU-intensive since it is called in such a repeated fashion, and is one of the main causes
 * the game lags in some maps packed with many entities.
 *
 * This patch uses a much bigger table, which enables performing a direct jump on the right offset of that jump
 * table directly instead of first checking inside the LUT what the jump offset for this entity is (if there is
 * one).
 */
class PatchOptimizeAILookup : public GamePatch
{
private:
    std::vector<std::pair<uint32_t, uint32_t>> _ai_addrs_for_enemy_id;
    uint32_t _ai_table_addr = 0xFFFFFFFF;

public:
    void load_from_rom(const md::ROM& rom) override
    {
        _ai_addrs_for_enemy_id.resize(0xFE, std::make_pair(0xFFFFFFFF, 0xFFFFFFFF));

        uint16_t i = 0;
        while(rom.get_word(offsets::ENEMY_AI_TABLE + (i*2)) != 0xFFFF)
        {
            uint16_t enemy_id = rom.get_word(offsets::ENEMY_AI_TABLE + (i*2));

            uint32_t branch_instruction_addr = offsets::ENEMY_AI_JUMP_TABLE + (i*8);
            int16_t jump_offset_A = (int16_t)rom.get_word(branch_instruction_addr + 2);
            uint32_t jump_destination_A = branch_instruction_addr + 2 + jump_offset_A;

            branch_instruction_addr += 4;
            int16_t jump_offset_B = (int16_t)rom.get_word(branch_instruction_addr + 2);
            uint32_t jump_destination_B = branch_instruction_addr + 2 + jump_offset_B;

            _ai_addrs_for_enemy_id[enemy_id] = std::make_pair(jump_destination_A, jump_destination_B);
            ++i;
        }
    }

    void clear_space_in_rom(md::ROM& rom) override
    {
        rom.mark_empty_chunk(offsets::ENEMY_AI_TABLE, offsets::ENEMY_AI_JUMP_TABLE_END);
    }

    void inject_data(md::ROM& rom, World& world) override
    {
        ByteArray ai_branch_table_bytes;
        for(auto& [addr_A, addr_B] : _ai_addrs_for_enemy_id)
        {
            ai_branch_table_bytes.add_long(addr_A);
            ai_branch_table_bytes.add_long(addr_B);
        }

        _ai_table_addr = rom.inject_bytes(ai_branch_table_bytes);
    }

    void inject_code(md::ROM& rom, World& world) override
    {
        md::Code func;
        {
            func.moveq(0, reg_D0);
            // Store entity ID in D0, and multiply it by 8 to form an offset
            func.moveb(addr_(reg_A5, 0x3B), reg_D0);
            func.addw(reg_D0, reg_D0);
            func.addw(reg_D0, reg_D0);
            func.addw(reg_D0, reg_D0);
            // Add "offset" D1 (initialized externally, either 0 or 4) to get AI variant A or B.
            func.addw(reg_D1, reg_D0);
            // Fetch the address from the AI jump table, storing it in D0 to save a CMPA
            func.lea(_ai_table_addr, reg_A0);
            func.movel(addr_(reg_A0, reg_D0), reg_D0);
            func.bmi("default_address");
            {
                // Put the address back in an address register, and jump on it
                func.movel(reg_D0, reg_A0);
                func.moveq(0, reg_D0);
                func.jmp(addr_(reg_A0));
            }
            func.label("default_address");
            {
                func.jsr(0x1A8ADA); // LoadSpecialAI
                func.rts();
            }
        }

        rom.set_code(0x1A83F0, func);
        if(0x1A83F0 + func.get_bytes().size() > offsets::ENEMY_AI_TABLE)
            throw LandstalkerException("Replacement function is too large and is overlapping other code!");
    }
};
