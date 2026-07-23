#include "loadrace.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/esm/attr.hpp>
#include <components/esm3/loadskil.hpp>

namespace ESM
{
    int32_t Race::RADTstruct::getAttribute(ESM::RefId attribute, bool male) const
    {
        const auto it = mAttributeValues.find(attribute);
        if (it == mAttributeValues.end())
            return 0;
        if (male)
            return it->second.mMale;
        return it->second.mFemale;
    }

    void Race::RADTstruct::setAttribute(ESM::RefId attribute, bool male, int32_t value)
    {
        auto& values = mAttributeValues[attribute];
        if (male)
            values.mMale = value;
        else
            values.mFemale = value;
    }

    void Race::AttributeValues::load(ESMReader& esm)
    {
        esm.getT(mMale);
        esm.getT(mFemale);
    }

    void Race::AttributeValues::save(ESMWriter& esm) const
    {
        esm.writeT(mMale);
        esm.writeT(mFemale);
    }

    void Race::RADTstruct::load(ESMReader& esm)
    {
        esm.getSubHeader();
        for (auto& bonus : mBonus)
        {
            int32_t skill;
            esm.getT(skill);
            bonus.mSkill = ESM::Skill::indexToRefId(skill);
            esm.getT(bonus.mBonus);
        }
        for (int i = 0; i < ESM::Attribute::Length; ++i)
            mAttributeValues[ESM::Attribute::indexToRefId(i)].load(esm);
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
            int32_t skill = ESM::Skill::refIdToIndex(bonus.mSkill);
            esm.writeT(skill);
            esm.writeT(bonus.mBonus);
        }
        for (int i = 0; i < ESM::Attribute::Length; ++i)
        {
            const auto it = mAttributeValues.find(ESM::Attribute::indexToRefId(i));
            if (it == mAttributeValues.end())
                AttributeValues{}.save(esm);
            else
                it->second.save(esm);
        }
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
            bonus.mSkill = {};
            bonus.mBonus = 0;
        }

        mData.mAttributeValues.clear();

        mData.mMaleHeight = mData.mFemaleHeight = 1;
        mData.mMaleWeight = mData.mFemaleWeight = 1;

        mData.mFlags = 0;
    }
}
