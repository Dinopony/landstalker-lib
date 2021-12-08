#include "../md_tools.hpp"
#include "../model/world.hpp"

void alter_fahl_challenge(md::ROM& rom, const World& world)
{
    // Neutralize mid-challenge proposals for money
    rom.set_code(0x12D52, md::Code().nop(24));

    // Set the end of the challenge at the number of fahl enemies in the world list
    rom.set_byte(0x12D87, (uint8_t)world.fahl_enemies().size());
}
