#pragma once

#include "../game_patch.hpp"
#include "../../constants/offsets.hpp"

class PatchOptimizeCollisionDetect : public GamePatch
{
public:
    void inject_code(md::ROM& rom, World& world) override
    {
        constexpr uint8_t HitBoxXStart = 24;
        constexpr uint8_t HitBoxXEnd = 26;
        constexpr uint8_t HitBoxYStart = 28;
        constexpr uint8_t HitBoxYEnd = 30;
        constexpr uint8_t HitBoxZStart = 18;
        constexpr uint8_t HitBoxZEnd = 0x54;

        md::Code func;
        {
            func.movem_to_stack({}, { reg_A2 });
            func.lea(0xFF5400, reg_A0);                     // A0 points on currently tested entity (starts on first entity)
            func.lea(addrw_(reg_A0, reg_D0), reg_A2);       // A2 points on input entity
<
            // Setup values in registers
            func.lea(addr_(reg_A2, HitBoxXStart), reg_A1);
            func.movew(addr_postinc_(reg_A1), reg_D1);                  // Bounding box min X
            func.movew(addr_postinc_(reg_A1), reg_D2);                  // Bounding box max X
            func.movew(addr_postinc_(reg_A1), reg_D3);                  // Bounding box min Y
            func.movew(addr_(reg_A1), reg_D4);                          // Bounding box max Y
            func.movew(addrw_(reg_A0, reg_D0, HitBoxZStart), reg_D5);   // Bounding box min Z
            func.movew(addrw_(reg_A0, reg_D0, HitBoxZEnd), reg_D6);     // Bounding box max Z

            // func.movel(reg_A0, reg_A1);

            func.label("loop");
            {
                // Do not compare against ourselves
                func.cmpa(reg_A0, reg_A2);
                func.beq("next_loop");

                // If entity starts with 0xFFFF, we reached end of table
                func.tstw(addr_(reg_A0));
                func.bmi("exit_no_collision");

                func.cmpw(addr_(reg_A0, HitBoxYStart), reg_D4);
                func.bcs("next_loop");

                func.cmpw(addr_(reg_A0, HitBoxXStart), reg_D2);
                func.bcs("next_loop");

                func.cmpw(addr_(reg_A0, HitBoxZStart), reg_D6);
                func.bcs("next_loop");

                func.cmpw(addr_(reg_A0, HitBoxXEnd), reg_D1);
                func.bhi("next_loop");

                func.cmpw(addr_(reg_A0, HitBoxYEnd), reg_D3);
                func.bhi("next_loop");

                func.cmpw(addr_(reg_A0, HitBoxZEnd), reg_D5);
                func.bhi("next_loop");

                // func.movel(reg_A0, reg_A1);

                // Check if entity has no collisions
                func.tstb(addr_(reg_A0, 0x8));
                func.bne("next_loop");

                // Check if entity is already known to be right underneath us
                func.cmpw(addr_(reg_A0, 0x30), reg_D0);
                func.beq("next_loop");

                // Exit
                func.ori_to_ccr(0x01);
                func.bra("ret");

                func.label("next_loop");
                func.lea(addr_(reg_A0, 0x80), reg_A0);
                func.bra("loop");
            }

            func.label("exit_no_collision");
            func.tstb(reg_D0);
            func.label("ret");
            func.movem_from_stack({}, { reg_A2 });
            func.rts();
        }

        rom.set_code(0x2F76, func); // Replace original CollisionDetect function
        if(0x2F76 + func.get_bytes().size() > 0x2FEA)
            throw std::exception("Replacement function is too large and is overlapping other code!");
    }
};
