#include "loadbody.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/concepts.hpp>

namespace ESM
{
    template <Misc::SameAsWithoutCvref<BodyPart::BYDTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mPart, v.mVampire, v.mFlags, v.mType);
    }

    void BodyPart::load(ESMReader& esm, bool& isDeleted)
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
                    mRace = esm.getRefId();
                    break;
                case fourCC("BYDT"):
                    esm.getSubComposite(mData);
                    hasData = true;
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
            esm.fail("Missing BYDT subrecord");
    }

    void BodyPart::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCRefId("FNAM", mRace);
        esm.writeNamedComposite("BYDT", mData);
    }

    void BodyPart::blank()
    {
        mRecordFlags = 0;
        mData.mPart = 0;
        mData.mVampire = 0;
        mData.mFlags = 0;
        mData.mType = 0;

        mModel.clear();
        mRace = ESM::RefId();
    }

    bool isFirstPersonBodyPart(const BodyPart& value)
    {
        return value.mId.endsWith("1st");
    }
}
