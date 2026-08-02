#include "loadingr.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/esm/attr.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/misc/concepts.hpp>

namespace ESM
{
    namespace
    {
        // IRDT format defined by Morrowind.esm
        struct EsmIRDTstruct
        {
            float mWeight;
            int32_t mValue, mEffectID[4], mSkills[4], mAttributes[4];
        };

        // IRDT format used in saves
        struct ValueAndWeight
        {
            float mWeight;
            int32_t mValue;
        };

        void toBinary(const Ingredient::IRDTstruct& src, EsmIRDTstruct& dst)
        {
            dst.mWeight = src.mWeight;
            dst.mValue = src.mValue;
            for (int i = 0; i < 4; ++i)
            {
                dst.mEffectID[i] = ESM::MagicEffect::refIdToIndex(src.mEffectID[i]);
                dst.mSkills[i] = ESM::Skill::refIdToIndex(src.mSkills[i]);
                dst.mAttributes[i] = ESM::Attribute::refIdToIndex(src.mAttributes[i]);
            }
        }

        void fromBinary(const EsmIRDTstruct& src, Ingredient::IRDTstruct& dst)
        {
            dst.mWeight = src.mWeight;
            dst.mValue = src.mValue;
            for (int i = 0; i < 4; ++i)
            {
                dst.mEffectID[i] = ESM::MagicEffect::indexToRefId(src.mEffectID[i]);
                dst.mSkills[i] = ESM::Skill::indexToRefId(src.mSkills[i]);
                dst.mAttributes[i] = ESM::Attribute::indexToRefId(src.mAttributes[i]);
            }
        }
    }

    template <Misc::SameAsWithoutCvref<EsmIRDTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mWeight, v.mValue, v.mEffectID, v.mSkills, v.mAttributes);
    }

    template <Misc::SameAsWithoutCvref<ValueAndWeight> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mWeight, v.mValue);
    }

    void Ingredient::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

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
                case fourCC("MODL"):
                    mModel = esm.getHString();
                    break;
                case fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case fourCC("IRDT"):
                {
                    if (esm.getFormatVersion() <= MaxIngredientIndexFormatVersion)
                    {
                        EsmIRDTstruct bin;
                        esm.getSubComposite(bin);
                        fromBinary(bin, mData);
                    }
                    else
                    {
                        ValueAndWeight bin;
                        esm.getSubComposite(bin);
                        mData.mWeight = bin.mWeight;
                        mData.mValue = bin.mValue;
                        for (int i = 0; i < 4 && esm.isNextSub("ENID"); ++i)
                        {
                            mData.mEffectID[i] = esm.getRefId();
                            mData.mSkills[i] = esm.getHNORefId("ENSK");
                            mData.mAttributes[i] = esm.getHNORefId("ENAT");
                        }
                    }
                    hasData = true;
                    break;
                }
                case fourCC("SCRI"):
                    mScript = esm.getRefId();
                    break;
                case fourCC("ITEX"):
                    mIcon = esm.getHString();
                    break;
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
            esm.fail("Missing IRDT subrecord");

        if (esm.getFormatVersion() <= MaxIngredientIndexFormatVersion)
        {
            // horrible hack to fix broken data in records
            for (int i = 0; i < 4; ++i)
            {
                if (mData.mEffectID[i] != ESM::MagicEffect::AbsorbAttribute
                    && mData.mEffectID[i] != ESM::MagicEffect::DamageAttribute
                    && mData.mEffectID[i] != ESM::MagicEffect::DrainAttribute
                    && mData.mEffectID[i] != ESM::MagicEffect::FortifyAttribute
                    && mData.mEffectID[i] != ESM::MagicEffect::RestoreAttribute)
                {
                    mData.mAttributes[i] = ESM::RefId();
                }

                // is this relevant in cycle from 0 to 4?
                if (mData.mEffectID[i] != ESM::MagicEffect::AbsorbSkill
                    && mData.mEffectID[i] != ESM::MagicEffect::DamageSkill
                    && mData.mEffectID[i] != ESM::MagicEffect::DrainSkill
                    && mData.mEffectID[i] != ESM::MagicEffect::FortifySkill
                    && mData.mEffectID[i] != ESM::MagicEffect::RestoreSkill)
                {
                    mData.mSkills[i] = ESM::RefId();
                }
            }
        }
    }

    void Ingredient::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel.getOriginal());
        esm.writeHNOCString("FNAM", mName);
        if (esm.getFormatVersion() <= MaxIngredientIndexFormatVersion)
        {
            EsmIRDTstruct bin;
            toBinary(mData, bin);
            esm.writeNamedComposite("IRDT", bin);
        }
        else
        {
            ValueAndWeight bin{ .mWeight = mData.mWeight, .mValue = mData.mValue };
            esm.writeNamedComposite("IRDT", bin);
            int max = 4;
            while (max > 0 && mData.mEffectID[max - 1].empty())
                --max;
            for (int i = 0; i < max; ++i)
            {
                esm.writeHNRefId("ENID", mData.mEffectID[i]);
                esm.writeHNORefId("ENSK", mData.mSkills[i]);
                esm.writeHNORefId("ENAT", mData.mAttributes[i]);
            }
        }
        esm.writeHNOCRefId("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon.getOriginal());
    }

    void Ingredient::blank()
    {
        mRecordFlags = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        for (int i = 0; i < 4; ++i)
        {
            mData.mEffectID[i] = ESM::MagicEffect::WaterBreathing;
            mData.mSkills[i] = ESM::RefId();
            mData.mAttributes[i] = ESM::RefId();
        }

        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript = ESM::RefId();
    }
}
