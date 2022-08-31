#pragma once

#include "landstalker_lib/patches/game_patch.hpp"
#include "landstalker_lib/model/entity_type.hpp"

/**
 * In original game, bosses have way higher HP than their real HP pool, and the game checks regularly
 * if their health goes below 0x100 to trigger a death cutscene. In an effort to normalize bosses HP
 * and allow us to make them have bigger HP pools, we lower all of those checks to verify that health
 * goes below 0x002 while putting a special procedure to ensure they are unkillable by the player
 */
class PatchNormalizeBossesHP : public GamePatch
{
public:
    void alter_rom(md::ROM& rom) override
    {
        constexpr uint16_t NEW_THRESHOLD = 0x0002;

        // Make the HP check for unkillable enemies below NEW_THRESHOLD instead of below 0x6400
        rom.set_byte(0x118DC, NEW_THRESHOLD);
        rom.set_byte(0x118EC, NEW_THRESHOLD);
        rom.set_word(0x11D38, NEW_THRESHOLD);
        rom.set_byte(0x11D80, NEW_THRESHOLD);
        rom.set_byte(0x11F8A, NEW_THRESHOLD);
        rom.set_byte(0x11FAA, NEW_THRESHOLD);
        rom.set_byte(0x12072, NEW_THRESHOLD);
        rom.set_byte(0x120C0, NEW_THRESHOLD);
    }

    void alter_world(World& world) override
    {
        std::vector<std::string> bosses = {
                "miro_1" , "duke" , "mir" , "zak" , "spinner_white" , "miro_2" , "gola" , "nole"
        };

        for(const std::string& boss_name : bosses)
        {
            EntityEnemy* enemy = reinterpret_cast<EntityEnemy*>(world.entity_type(boss_name));
            enemy->unkillable(true);
            enemy->health(enemy->health() - 100);
        }
    }

    void inject_code(md::ROM& rom, World& world) override
    {
        // Call the injected function when killing an enemy
        uint32_t func_improved_checks = inject_func_improved_checks_before_kill(rom);
        rom.set_code(0x01625C, md::Code().jmp(func_improved_checks));
    }

private:
    /**
     * The original game has no way to make enemies unkillable, so it uses weird ultra high HP bosses
     * which don't represent their real life pool. This injection modifies the way this works to improve
     * this part of the engine by implementing a way to make them unkillable.
     */
    static uint32_t inject_func_improved_checks_before_kill(md::ROM& rom)
    {
        constexpr uint32_t ADDR_JMP_KILL = 0x16284;
        constexpr uint32_t ADDR_JMP_NO_KILL_YET = 0x16262;
        constexpr uint8_t DUKE_SPRITE_ID = 0x0C;

        // Inject a new function which fixes the money value check on an enemy when it is killed, causing the tree glitch to be possible
        md::Code func;
        {
            // If enemy doesn't hold money, no kill yet
            func.tstb(addr_(reg_A5, 0x36));
            func.beq("no_money");
            {
                func.jmp(ADDR_JMP_KILL);
            }
            func.label("no_money");
            // If enemy has "empty item" as a drop, make it unkillable
            func.cmpib(0x3F, addr_(reg_A5, 0x77));
            func.bne("no_kill_yet");
            {
                func.movew(0x0001, addr_(reg_A5, 0x3E)); // Set life back to 0x0001 to prevent death
                func.cmpib(DUKE_SPRITE_ID, addr_(reg_A5, 0x0B));
                func.bne("not_duke");
                {
                    func.moveb(0x21, addr_(reg_A5, 0x37)); // Interrupt Duke's pattern
                }
                func.label("not_duke");
                func.rts();
            }
            func.label("no_kill_yet");
            func.jmp(ADDR_JMP_NO_KILL_YET);
        }

        return rom.inject_code(func);
    }
};
