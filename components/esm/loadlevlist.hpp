#ifndef OPENMW_ESM_LEVLISTS_H
#define OPENMW_ESM_LEVLISTS_H

#include <string>
#include <vector>

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Leveled lists. Since these have identical layout, I only bothered
 * to implement it once.
 *
 * We should later implement the ability to merge leveled lists from
 * several files.
 */

struct LeveledListBase
{
    int mFlags;
    unsigned char mChanceNone; // Chance that none are selected (0-100)
    std::string mId;

    // Record name used to read references. Must be set before load() is
    // called.
    const char *mRecName;

    struct LevelItem
    {
        std::string mId;
        short mLevel;
    };

    std::vector<LevelItem> mList;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};

struct CreatureLevList: LeveledListBase
{
    static unsigned int sRecordId;

    enum Flags
    {

        AllLevels = 0x01  // Calculate from all levels <= player
                          // level, not just the closest below
                          // player.
    };

    CreatureLevList()
    {
        mRecName = "CNAM";
    }
};

struct ItemLevList: LeveledListBase
{
    static unsigned int sRecordId;

    enum Flags
    {

        Each = 0x01,      // Select a new item each time this
                          // list is instantiated, instead of
                          // giving several identical items
                          // (used when a container has more
                          // than one instance of one leveled
                          // list.)
        AllLevels = 0x02  // Calculate from all levels <= player
                          // level, not just the closest below
                          // player.
    };

    ItemLevList()
    {
        mRecName = "INAM";
    }
};

}
#endif
