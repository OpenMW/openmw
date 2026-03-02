#include "loadfact.hpp"

#include <stdexcept>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    int32_t& Faction::FADTstruct::getSkill(size_t index, bool)
    {
        return mSkills.at(index);
    }

    int32_t Faction::FADTstruct::getSkill(size_t index, bool) const
    {
        return mSkills.at(index);
    }

    void RankData::load(ESMReader& esm)
    {
        esm.getT(mAttribute1);
        esm.getT(mAttribute2);
        esm.getT(mPrimarySkill);
        esm.getT(mFavouredSkill);
        esm.getT(mFactReputation);
    }

    void RankData::save(ESMWriter& esm) const
    {
        esm.writeT(mAttribute1);
        esm.writeT(mAttribute2);
        esm.writeT(mPrimarySkill);
        esm.writeT(mFavouredSkill);
        esm.writeT(mFactReputation);
    }

    void Faction::FADTstruct::load(ESMReader& esm)
    {
        esm.getSubHeader();
        esm.getT(mAttribute);
        for (auto& rank : mRankData)
            rank.load(esm);
        esm.getT(mSkills);
        esm.getT(mIsHidden);
        if (mIsHidden > 1)
            esm.fail("Unknown flag!");
    }

    void Faction::FADTstruct::save(ESMWriter& esm) const
    {
        esm.startSubRecord("FADT");
        esm.writeT(mAttribute);
        for (const auto& rank : mRankData)
            rank.save(esm);
        esm.writeT(mSkills);
        esm.writeT(mIsHidden);
        esm.endRecord("FADT");
    }

    void Faction::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mReactions.clear();
        for (auto& rank : mRanks)
            rank.clear();

        size_t rankCounter = 0;
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
                    if (rankCounter >= mRanks.size())
                        esm.fail("Rank out of range");
                    mRanks[rankCounter++] = esm.getHString();
                    break;
                case fourCC("FADT"):
                    mData.load(esm);
                    hasData = true;
                    break;
                case fourCC("ANAM"):
                {
                    ESM::RefId faction = esm.getRefId();
                    int32_t reaction;
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

        for (const auto& rank : mRanks)
        {
            if (rank.empty())
                break;

            esm.writeHNString("RNAM", rank, 32);
        }

        mData.save(esm);

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
            mData.mRankData[i].mFactReputation = 0;

            mRanks[i].clear();
        }

        mData.mSkills.fill(0);

        mReactions.clear();
    }
}
