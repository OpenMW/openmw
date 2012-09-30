#ifndef OPENMW_ESM_FACT_H
#define OPENMW_ESM_FACT_H

#include <string>
#include <vector>

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Faction definitions
 */

// Requirements for each rank
struct RankData
{
    int mAttribute1, mAttribute2; // Attribute level

    int mSkill1, mSkill2; // Skill level (faction skills given in
    // skillID below.) You need one skill at
    // level 'skill1' and two skills at level
    // 'skill2' to advance to this rank.

    int mFactReaction; // Reaction from faction members
};

struct Faction
{
    std::string mId, mName;

    struct FADTstruct
    {
        // Which attributes we like
        int mAttribute1, mAttribute2;

        RankData mRankData[10];

        int mSkillID[6]; // IDs of skills this faction require
        int mUnknown; // Always -1?
        int mIsHidden; // 1 - hidden from player
    }; // 240 bytes

    FADTstruct mData;

    struct Reaction
    {
        std::string mFaction;
        int mReaction;
    };

    std::vector<Reaction> mReactions;

    // Name of faction ranks (may be empty for NPC factions)
    std::string mRanks[10];

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
