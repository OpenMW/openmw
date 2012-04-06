#ifndef _ESM_LEVLISTS_H
#define _ESM_LEVLISTS_H

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

/*
 * Leveled lists. Since these have identical layout, I only bothered
 * to implement it once.
 *
 * We should later implement the ability to merge leveled lists from
 * several files. 
 */

struct LeveledListBase
{
    enum Flags
    {
        AllLevels = 0x01, // Calculate from all levels <= player
                          // level, not just the closest below
                          // player.
        Each = 0x02       // Select a new item each time this
                          // list is instantiated, instead of
                          // giving several identical items
    };                    // (used when a container has more
                          // than one instance of one leveled
                          // list.)
    int flags;
    unsigned char chanceNone; // Chance that none are selected (0-255?)

    // Record name used to read references. Must be set before load() is
    // called.
    const char *recName;

    struct LevelItem
    {
        std::string id;
        short level;
    };

    std::vector<LevelItem> list;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};

struct CreatureLevList: LeveledListBase
{
    CreatureLevList()
    {
        recName = "CNAM";
    }
};

struct ItemLevList: LeveledListBase
{
    ItemLevList()
    {
        recName = "INAM";
    }
};

}
#endif
