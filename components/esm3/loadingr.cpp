#include "loadingr.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/esm3/loadmgef.hpp>
#include <components/misc/concepts.hpp>

namespace ESM
{
    template <typename T>
    constexpr bool loading = !std::is_const_v<std::remove_reference_t<T>>;

    template <Misc::SameAsWithoutCvref<Ingredient::IRDTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        int32_t ioEffectID[4];
        std::transform(
            std::begin(v.mEffectID), std::end(v.mEffectID), std::begin(ioEffectID), ESM::MagicEffect::refIdToIndex);
        f(v.mWeight, v.mValue, ioEffectID, v.mSkills, v.mAttributes);
        if constexpr (loading<T>)
        {
            std::transform(
                std::begin(ioEffectID), std::end(ioEffectID), std::begin(v.mEffectID), ESM::MagicEffect::indexToRefId);
        }
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
                    esm.getSubComposite(mData);
                    hasData = true;
                    break;
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

        // horrible hack to fix broken data in records
        for (int i = 0; i < 4; ++i)
        {
            if (mData.mEffectID[i] != ESM::MagicEffect::AbsorbAttribute &&
                mData.mEffectID[i] != ESM::MagicEffect::DamageAttribute &&
                mData.mEffectID[i] != ESM::MagicEffect::DrainAttribute &&
                mData.mEffectID[i] != ESM::MagicEffect::FortifyAttribute &&
                mData.mEffectID[i] != ESM::MagicEffect::RestoreAttribute)
            {
                mData.mAttributes[i] = -1;
            }

            // is this relevant in cycle from 0 to 4?
            if (mData.mEffectID[i] != ESM::MagicEffect::AbsorbSkill &&
                mData.mEffectID[i] != ESM::MagicEffect::DamageSkill &&
                mData.mEffectID[i] != ESM::MagicEffect::DrainSkill &&
                mData.mEffectID[i] != ESM::MagicEffect::FortifySkill &&
                mData.mEffectID[i] != ESM::MagicEffect::RestoreSkill)
            {
                mData.mSkills[i] = -1;
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

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeNamedComposite("IRDT", mData);
        esm.writeHNOCRefId("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon);
    }

    void Ingredient::blank()
    {
        mRecordFlags = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        for (int i = 0; i < 4; ++i)
        {
            mData.mEffectID[i] = ESM::MagicEffect::WaterBreathing;
            mData.mSkills[i] = 0;
            mData.mAttributes[i] = 0;
        }

        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript = ESM::RefId();
    }
}
