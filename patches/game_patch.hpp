#pragma once

#include "../md_tools.hpp"
#include "../model/world.hpp"

class GamePatch
{
public:
    GamePatch() = default;

    /**
     * Virtual function which performs all loadings that have to be done from the vanilla ROM when it is first loaded.
     * This function is called first, and when it is called, the ROM is ensured to be in its initial state.
     *
     * @param rom the ROM to read from
     */
    virtual void load_from_rom(const md::ROM& rom) {}

    /**
     * Virtual function designed to edit data / bytes in the original ROM.
     * This function is called second to be able to have a mechanism which checks if edited data is then
     * overwritten / cleared elsewhere down the road, meaning we have either dead edits or patch conflicts.
     *
     * @param rom
     */
    virtual void alter_rom(md::ROM& rom) {}

    /**
     * Virtual function designed to edit the global World object which is used to describe the game content.
     * This function is called third.
     *
     * @param rom
     */
    virtual void alter_world(World& world) {}

    /**
     * Virtual function which performs the "cleanup" logic after having read everything that was necessary.
     * This function is called fourth, which means any loading on data that would be erased in here needs to
     * be performed in `load_from_rom`.
     *
     * @param rom
     */
    virtual void clear_space_in_rom(md::ROM& rom) {}

    /**
     * Virtual function which injects data inside the ROM.
     * This function is called fifth, before any code is injected (allowing to use the address where this data was
     * injected inside the code).
     *
     * @param rom
     * @param world
     */
    virtual void inject_data(md::ROM& rom, World& world) {}

    /**
     * Virtual function which inject code inside the ROM.
     * This function is called sixth.
     *
     * @param rom
     */
    virtual void inject_code(md::ROM& rom, World& world) {}

    /**
     * Virtual function which enables doing stuff after everything else, ensuring all data & code was injected by
     * every patch. This function is called last.
     *
     * @param rom
     */
    virtual void postprocess(md::ROM& rom, const World& world) {}
};