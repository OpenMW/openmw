#include "loadrace.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/esm/attr.hpp>

namespace ESM
{
    int32_t Race::RADTstruct::getAttribute(ESM::RefId attribute, bool male) const
    {
        int index = ESM::Attribute::refIdToIndex(attribute);
        if (index < 0)
            return 0;
        index *= 2;
        if (!male)
            index++;
        return mAttributeValues[static_cast<size_t>(index)];
    }

    void Race::RADTstruct::setAttribute(ESM::RefId attribute, bool male, int32_t value)
    {
        int index = ESM::Attribute::refIdToIndex(attribute);
        if (index < 0)
            return;
        index *= 2;
        if (!male)
            index++;
        mAttributeValues[static_cast<size_t>(index)] = value;
    }

    void Race::RADTstruct::load(ESMReader& esm)
    {
        esm.getSubHeader();
        for (auto& bonus : mBonus)
        {
            esm.getT(bonus.mSkill);
            esm.getT(bonus.mBonus);
        }
        esm.getT(mAttributeValues);
        esm.getT(mMaleHeight);
        esm.getT(mFemaleHeight);
        esm.getT(mMaleWeight);
        esm.getT(mFemaleWeight);
        esm.getT(mFlags);
    }

    void Race::RADTstruct::save(ESMWriter& esm) const
    {
        esm.startSubRecord("RADT");
        for (const auto& bonus : mBonus)
        {
            esm.writeT(bonus.mSkill);
            esm.writeT(bonus.mBonus);
        }
        esm.writeT(mAttributeValues);
        esm.writeT(mMaleHeight);
        esm.writeT(mFemaleHeight);
        esm.writeT(mMaleWeight);
        esm.writeT(mFemaleWeight);
        esm.writeT(mFlags);
        esm.endRecord("RADT");
    }

    void Race::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mPowers.mList.clear();

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
                case fourCC("RADT"):
                    mData.load(esm);
                    hasData = true;
                    break;
                case fourCC("DESC"):
                    mDescription = esm.getHString();
                    break;
                case fourCC("NPCS"):
                    mPowers.add(esm);
                    break;
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasData && !isDeleted)
            esm.fail("Missing RADT subrecord");
    }
    void Race::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNOCString("FNAM", mName);
        mData.save(esm);
        mPowers.save(esm);
        esm.writeHNOString("DESC", mDescription);
    }

    void Race::blank()
    {
        mRecordFlags = 0;
        mName.clear();
        mDescription.clear();

        mPowers.mList.clear();

        for (auto& bonus : mData.mBonus)
        {
            bonus.mSkill = -1;
            bonus.mBonus = 0;
        }

        mData.mAttributeValues.fill(1);

        mData.mMaleHeight = mData.mFemaleHeight = 1;
        mData.mMaleWeight = mData.mFemaleWeight = 1;

        mData.mFlags = 0;
    }
}
