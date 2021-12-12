#include "../md_tools.hpp"

void make_lifestocks_give_specific_health(md::ROM& rom, uint8_t health_per_lifestock)
{
    uint16_t health_per_lifestock_formatted = health_per_lifestock << 8;
    rom.set_word(0x291E2, health_per_lifestock_formatted);
    rom.set_word(0x291F2, health_per_lifestock_formatted);
    rom.set_word(0x7178, health_per_lifestock_formatted);
    rom.set_word(0x7188, health_per_lifestock_formatted);
}