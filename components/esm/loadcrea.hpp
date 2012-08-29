#ifndef _ESM_CREA_H
#define _ESM_CREA_H

#include "esm_reader.hpp"
#include "loadcont.hpp"
#include "defs_ai.hpp"

namespace ESM
{

/*
 * Creature definition
 *
 */

struct Creature
{
    // Default is 0x48?
    enum Flags
    {
        Biped = 0x001, Respawn = 0x002, Weapon = 0x004, // Has weapon and shield
        None = 0x008, // ??
        Swims = 0x010,
        Flies = 0x020, // Don't know what happens if several
        Walks = 0x040, // of these are set
        Essential = 0x080,
        Skeleton = 0x400, // Does not have normal blood
        Metal = 0x800
    // Has 'golden' blood
    };

    enum Type
    {
        Creatures = 0,
        Deadra = 1,
        Undead = 2,
        Humanoid = 3
    };

    struct NPDTstruct
    {
        int type;
        // For creatures we obviously have to use ints, not shorts and
        // bytes like we use for NPCs.... this file format just makes so
        // much sense! (Still, _much_ easier to decode than the NIFs.)
        int level;
        int strength, intelligence, willpower, agility, speed, endurance,
                personality, luck, health, mana, fatigue; // Stats
        int soul; // The creatures soul value (used with soul gems.)
        int combat, magic, stealth; // Don't know yet.
        int attack[6]; // AttackMin1, AttackMax1, ditto2, ditto3
        int gold;
    }; // 96 bytes

    NPDTstruct data;

    int flags;
    float scale;

    std::string model, name, script, original; // Base creature that this is a modification of

    // Defined in loadcont.hpp
    InventoryList inventory;

    bool hasAI;
    AIDTstruct AI;

    std::string mId;

    void load(ESMReader &esm, const std::string& id);
};
}
#endif
