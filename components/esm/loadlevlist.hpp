#ifndef OPENMW_ESM_LEVLISTS_H
#define OPENMW_ESM_LEVLISTS_H

#include <string>
#include <vector>

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Levelled lists. Since these have identical layout, I only bothered
 * to implement it once.
 *
 * We should later implement the ability to merge levelled lists from
 * several files.
 */

struct LevelledListBase
{
    int mFlags;
    unsigned char mChanceNone; // Chance that none are selected (0-100)
    unsigned int mRecordFlags;
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

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};

struct CreatureLevList: LevelledListBase
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "CreatureLevList"; }

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

struct ItemLevList: LevelledListBase
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "ItemLevList"; }

    enum Flags
    {

        Each = 0x01,      // Select a new item each time this
                          // list is instantiated, instead of
                          // giving several identical items
                          // (used when a container has more
                          // than one instance of one levelled
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
