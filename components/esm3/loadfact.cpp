#include "loadfact.hpp"

#include <stdexcept>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    int& Faction::FADTstruct::getSkill(int index, bool)
    {
        return mSkills.at(index);
    }

    int Faction::FADTstruct::getSkill(int index, bool) const
    {
        return mSkills.at(index);
    }

    void Faction::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mReactions.clear();
        for (int i = 0; i < 10; ++i)
            mRanks[i].clear();

        int rankCounter = 0;
        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case SREC_NAME:
                    mId = esm.getRefId();
                    hasName = true;
                    break;
                case fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case fourCC("RNAM"):
                    if (rankCounter >= 10)
                        esm.fail("Rank out of range");
                    mRanks[rankCounter++] = esm.getHString();
                    break;
                case fourCC("FADT"):
                    esm.getHTSized<240>(mData);
                    if (mData.mIsHidden > 1)
                        esm.fail("Unknown flag!");
                    hasData = true;
                    break;
                case fourCC("ANAM"):
                {
                    ESM::RefId faction = esm.getRefId();
                    int reaction;
                    esm.getHNT(reaction, "INTV");
                    // Prefer the lowest reaction in case a faction is listed multiple times
                    auto it = mReactions.find(faction);
                    if (it == mReactions.end())
                        mReactions.emplace(faction, reaction);
                    else if (it->second > reaction)
                        it->second = reaction;
                    break;
                }
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasData && !isDeleted)
            esm.fail("Missing FADT subrecord");
    }

    void Faction::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNOCString("FNAM", mName);

        for (int i = 0; i < 10; i++)
        {
            if (mRanks[i].empty())
                break;

            esm.writeHNString("RNAM", mRanks[i], 32);
        }

        esm.writeHNT("FADT", mData, 240);

        for (auto it = mReactions.begin(); it != mReactions.end(); ++it)
        {
            esm.writeHNRefId("ANAM", it->first);
            esm.writeHNT("INTV", it->second);
        }
    }

    void Faction::blank()
    {
        mRecordFlags = 0;
        mName.clear();
        mData.mAttribute.fill(0);
        mData.mIsHidden = 0;

        for (size_t i = 0; i < mData.mRankData.size(); ++i)
        {
            mData.mRankData[i].mAttribute1 = mData.mRankData[i].mAttribute2 = 0;
            mData.mRankData[i].mPrimarySkill = mData.mRankData[i].mFavouredSkill = 0;
            mData.mRankData[i].mFactReaction = 0;

            mRanks[i].clear();
        }

        mData.mSkills.fill(0);

        mReactions.clear();
    }
}
