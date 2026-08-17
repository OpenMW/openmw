#include "loadrace.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/esm/attr.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/misc/concepts.hpp>

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

    template <Misc::SameAsWithoutCvref<Race::AttributeValues> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mMale, v.mFemale);
    }

    template <Misc::SameAsWithoutCvref<Race::RADTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mMaleHeight, v.mFemaleHeight, v.mMaleWeight, v.mFemaleWeight, v.mFlags);
    }

    void Race::RADTstruct::load(ESMReader& esm)
    {
        esm.getSubHeader();
        if (esm.getFormatVersion() <= MaxFixedStatsFormatVersion)
        {
            for (auto& bonus : mBonus)
            {
                int32_t skill;
                esm.getT(skill);
                bonus.mSkill = ESM::Skill::indexToRefId(skill);
                esm.getT(bonus.mBonus);
            }
            for (int i = 0; i < ESM::Attribute::Length; ++i)
                esm.getComposite(mAttributeValues[ESM::Attribute::indexToRefId(i)]);
            esm.getT(mMaleHeight);
            esm.getT(mFemaleHeight);
            esm.getT(mMaleWeight);
            esm.getT(mFemaleWeight);
            esm.getT(mFlags);
        }
        else
        {
            esm.getComposite(*this);
            for (size_t i = 0; i < mBonus.size() && esm.isNextSub("SKIL"); ++i)
            {
                esm.getSubHeader();
                esm.getT(mBonus[i].mBonus);
                mBonus[i].mSkill = esm.getRefId(esm.getSubSize() - sizeof(mBonus[i].mBonus));
            }
            while (esm.isNextSub("ATTR"))
            {
                esm.getSubHeader();
                AttributeValues value;
                esm.getComposite(value);
                ESM::RefId attribute = esm.getRefId(esm.getSubSize() - getCompositeSize(value));
                mAttributeValues.emplace(attribute, value);
            }
        }
    }

    void Race::RADTstruct::save(ESMWriter& esm) const
    {
        if (esm.getFormatVersion() <= MaxFixedStatsFormatVersion)
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
                    esm.writeComposite(AttributeValues{});
                else
                    esm.writeComposite(it->second);
            }
            esm.writeT(mMaleHeight);
            esm.writeT(mFemaleHeight);
            esm.writeT(mMaleWeight);
            esm.writeT(mFemaleWeight);
            esm.writeT(mFlags);
            esm.endRecord("RADT");
        }
        else
        {
            esm.writeNamedComposite("RADT", *this);
            for (const auto& bonus : mBonus)
            {
                if (bonus.mSkill.empty())
                    continue;
                esm.startSubRecord("SKIL");
                esm.writeT(bonus.mBonus);
                esm.writeHRefId(bonus.mSkill);
                esm.endRecord("SKIL");
            }
            for (const auto& [attribute, value] : mAttributeValues)
            {
                if (attribute.empty() || (value.mFemale == 0 && value.mMale == 0))
                    continue;
                esm.startSubRecord("ATTR");
                esm.writeComposite(value);
                esm.writeHRefId(attribute);
                esm.endRecord("ATTR");
            }
        }
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

        mData.mBonus.fill({});

        mData.mAttributeValues.clear();

        mData.mMaleHeight = mData.mFemaleHeight = 1;
        mData.mMaleWeight = mData.mFemaleWeight = 1;

        mData.mFlags = 0;
    }
}
