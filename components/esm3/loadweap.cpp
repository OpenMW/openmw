#include "loadweap.hpp"

#include <components/esm/defs.hpp>
#include <components/misc/concepts.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    template <Misc::SameAsWithoutCvref<Weapon::WPDTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mWeight, v.mValue, v.mType, v.mHealth, v.mSpeed, v.mReach, v.mEnchant, v.mChop, v.mSlash, v.mThrust,
            v.mFlags);
    }

    void Weapon::load(ESMReader& esm, bool& isDeleted)
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
                case fourCC("WPDT"):
                    esm.getSubComposite(mData);
                    hasData = true;
                    break;
                case fourCC("SCRI"):
                    mScript = esm.getRefId();
                    break;
                case fourCC("ITEX"):
                    mIcon = esm.getHString();
                    break;
                case fourCC("ENAM"):
                    mEnchant = esm.getRefId();
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
            esm.fail("Missing WPDT subrecord");
    }
    void Weapon::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeNamedComposite("WPDT", mData);
        esm.writeHNOCRefId("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon);
        esm.writeHNOCRefId("ENAM", mEnchant);
    }

    void Weapon::blank()
    {
        mRecordFlags = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mType = 0;
        mData.mHealth = 0;
        mData.mSpeed = 0;
        mData.mReach = 0;
        mData.mEnchant = 0;
        mData.mChop.fill(0);
        mData.mSlash.fill(0);
        mData.mThrust.fill(0);
        mData.mFlags = 0;

        mName.clear();
        mModel.clear();
        mIcon.clear();
        mEnchant = ESM::RefId();
        mScript = ESM::RefId();
    }
}
