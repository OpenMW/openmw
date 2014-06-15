#include "loadfact.hpp"

#include <stdexcept>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Faction::sRecordId = REC_FACT;

    int& Faction::FADTstruct::getSkill (int index, bool ignored)
    {
        if (index<0 || index>=7)
            throw std::logic_error ("skill index out of range");

        return mSkills[index];
    }

    int Faction::FADTstruct::getSkill (int index, bool ignored) const
    {
        if (index<0 || index>=7)
            throw std::logic_error ("skill index out of range");

        return mSkills[index];
    }

void Faction::load(ESMReader &esm)
{
    mName = esm.getHNOString("FNAM");

    // Read rank names. These are optional.
    int i = 0;
    while (esm.isNextSub("RNAM") && i < 10)
        mRanks[i++] = esm.getHString();

    // Main data struct
    esm.getHNT(mData, "FADT", 240);

    if (mData.mIsHidden > 1)
        esm.fail("Unknown flag!");

    // Read faction response values
    while (esm.hasMoreSubs())
    {
        std::string faction = esm.getHNString("ANAM");
        int reaction;
        esm.getHNT(reaction, "INTV");
        mReactions[faction] = reaction;
    }
}
void Faction::save(ESMWriter &esm) const
{
    esm.writeHNOCString("FNAM", mName);

    for (int i = 0; i < 10; i++)
    {
        if (mRanks[i].empty())
            break;

        esm.writeHNString("RNAM", mRanks[i], 32);
    }

    esm.writeHNT("FADT", mData, 240);

    for (std::map<std::string, int>::const_iterator it = mReactions.begin(); it != mReactions.end(); ++it)
    {
        esm.writeHNString("ANAM", it->first);
        esm.writeHNT("INTV", it->second);
    }
}

    void Faction::blank()
    {
        mName.clear();
        mData.mAttribute[0] = mData.mAttribute[1] = 0;
        mData.mIsHidden = 0;

        for (int i=0; i<10; ++i)
        {
            mData.mRankData[i].mAttribute1 = mData.mRankData[i].mAttribute2 = 0;
            mData.mRankData[i].mSkill1 = mData.mRankData[i].mSkill2 = 0;
            mData.mRankData[i].mFactReaction = 0;

            mRanks[i].clear();
        }

        for (int i=0; i<7; ++i)
            mData.mSkills[i] = 0;

        mReactions.clear();
    }
}
