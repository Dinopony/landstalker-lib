#pragma once

#include "../game_patch.hpp"
#include "../../constants/offsets.hpp"

/**
 * On every frame, the game updates a "draw order list" starting at 0xFF11DE where the earlier an entity is listed,
 * the higher its priority is regarding drawing, e.g. the first entity of the list always gets drawn over all other
 * entities.
 */
class PatchOptimizeReorderDrawOrderList : public GamePatch
{
public:
    void inject_code(md::ROM& rom, World& world) override
    {
        constexpr uint8_t HitBoxXStart = 24;
        constexpr uint8_t HitBoxXEnd = 26;
        constexpr uint8_t HitBoxYStart = 28;
        constexpr uint8_t HitBoxYEnd = 30;
        constexpr uint8_t ZPos = 18;

        // A0 = address of the first entity inside draw order table being compared to others
        // A1 = address of the second entity inside draw order table being compared to A0

        md::Code func;
        func.movem_to_stack({ reg_D0_D7 }, { reg_A0_A6 });
        {
            func.lea(0xFF5400, reg_A5);
            func.lea(0xFF11DE, reg_A0);
            func.movew(0xFFFF, addr_(0xFF11FE)); // Put an "end" word at the end of the list

            // One turn of Loop_1 consists in comparing one A0 to eveyr subsequent A1. Loop_1 handles A0 value,
            // whereas changing A1 value is delegated to Loop_2.
            func.label("loop_entity_1");
            {
                // Put A1 one slot ahead of A0 to start the loop
                func.movel(reg_A0, reg_A1);
                func.addql(0x2, reg_A1);

                // Store A0 entity offset inside D0. If negative (0xFFFF), it means we reached the end of table.
                func.movew(addr_(reg_A0), reg_D0);
                func.bpl("no_ret");
                {
                    func.movem_from_stack({ reg_D0_D7 }, { reg_A0_A6 });
                    func.rts();
                }
                func.label("no_ret");

                func.lea(addrw_(reg_A5,reg_D0,HitBoxXStart), reg_A3);
                func.movew(addr_postinc_(reg_A3), reg_D2);
                func.movew(addr_postinc_(reg_A3), reg_D3);
                func.movew(addr_postinc_(reg_A3), reg_D4);
                func.movew(addr_(reg_A3), reg_D5);

                // Loop_2 consists looping A1 over every subsequent entity to A0 inside the table, comparing A0
                // to every following entity.
                func.label("loop_entity_2");
                {
                    // Store A1 entity offset inside D1. If negative (0xFFFF), it means we have finished comparing A0
                    // to all entities ahead of it, so we can increment A0 and go for another loop_1 turn.
                    func.movew(addr_(reg_A1), reg_D1);
                    func.bmi("next_loop_entity_1");

                    // If minX0 >= maxX1: D0 is in front of D1 -> order is good -> go to next entity
                    func.cmpw(addrw_(reg_A5,reg_D1,HitBoxXEnd), reg_D2);
                    func.bcc("next_loop_entity_2");

                    // If minY0 >= maxY1: D0 is in front of D1 -> order is good -> go to next entity
                    func.cmpw(addrw_(reg_A5,reg_D1,HitBoxYEnd), reg_D4);
                    func.bcc("next_loop_entity_2");

                    // If maxX0 < minX1: D1 is in front of D0 -> order is NOT good -> swap those two entities
                    func.cmpw(addrw_(reg_A5,reg_D1,HitBoxXStart), reg_D3);
                    func.bls("swap_a0_a1");

                    // If maxY0 < minY1: D1 is in front of D0 -> order is NOT good -> swap those two entities
                    func.cmpw(addrw_(reg_A5,reg_D1,HitBoxYStart), reg_D5);
                    func.bls("swap_a0_a1");

                    // If Z0 < Z1: D1 is in front of D0 -> order is NOT good -> swap those two entities
                    func.movew(addrw_(reg_A5,reg_D0,ZPos), reg_D6);
                    func.cmpw(addrw_(reg_A5,reg_D1,ZPos), reg_D6);
                    func.bcc("next_loop_entity_2");

                    func.label("swap_a0_a1");
                    {
                        // Swap A0 and A1 values
                        func.movew(reg_D1, addr_(reg_A0));
                        func.movew(reg_D0, addr_(reg_A1));

                        // Update D0 and related values
                        func.movew(reg_D1, reg_D0);

                        func.lea(addrw_(reg_A5,reg_D0,HitBoxXStart), reg_A3);
                        func.movew(addr_postinc_(reg_A3), reg_D2);
                        func.movew(addr_postinc_(reg_A3), reg_D3);
                        func.movew(addr_postinc_(reg_A3), reg_D4);
                        func.movew(addr_(reg_A3), reg_D5);
                    }
                    func.label("next_loop_entity_2");
                    func.addql(0x2, reg_A1);
                }
                func.bra("loop_entity_2");
            }
            func.label("next_loop_entity_1");
            func.addql(0x2, reg_A0);
            func.bra("loop_entity_1");
        }

        rom.set_code(0x49C0, func); // Replace original function
        if(0x49C0 + func.get_bytes().size() > 0x4A3A)
            throw LandstalkerException("Replacement function is too large and is overlapping other code!");
    }
};
