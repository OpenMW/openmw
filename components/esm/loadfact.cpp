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

    void Faction::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mReactions.clear();
        for (int i=0;i<10;++i)
            mRanks[i].clear();

        int rankCounter = 0;
        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'R','N','A','M'>::value:
                    if (rankCounter >= 10)
                        esm.fail("Rank out of range");
                    mRanks[rankCounter++] = esm.getHString();
                    break;
                case ESM::FourCC<'F','A','D','T'>::value:
                    esm.getHT(mData, 240);
                    if (mData.mIsHidden > 1)
                        esm.fail("Unknown flag!");
                    hasData = true;
                    break;
                case ESM::FourCC<'A','N','A','M'>::value:
                {
                    std::string faction = esm.getHString();
                    int reaction;
                    esm.getHNT(reaction, "INTV");
                    mReactions[faction] = reaction;
                    break;
                }
                case ESM::SREC_DELE:
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

    void Faction::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

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
            mData.mRankData[i].mPrimarySkill = mData.mRankData[i].mFavouredSkill = 0;
            mData.mRankData[i].mFactReaction = 0;

            mRanks[i].clear();
        }

        for (int i=0; i<7; ++i)
            mData.mSkills[i] = 0;

        mReactions.clear();
    }
}
