#include "loadingr.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/concepts.hpp>

namespace ESM
{
    template <Misc::SameAsWithoutCvref<Ingredient::IRDTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mWeight, v.mValue, v.mEffectID, v.mSkills, v.mAttributes);
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
            if (mData.mEffectID[i] != 85 && mData.mEffectID[i] != 22 && mData.mEffectID[i] != 17
                && mData.mEffectID[i] != 79 && mData.mEffectID[i] != 74)
            {
                mData.mAttributes[i] = -1;
            }

            // is this relevant in cycle from 0 to 4?
            if (mData.mEffectID[i] != 89 && mData.mEffectID[i] != 26 && mData.mEffectID[i] != 21
                && mData.mEffectID[i] != 83 && mData.mEffectID[i] != 78)
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
            mData.mEffectID[i] = 0;
            mData.mSkills[i] = 0;
            mData.mAttributes[i] = 0;
        }

        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript = ESM::RefId();
    }
}
