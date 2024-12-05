#include "loadappa.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/concepts.hpp>

namespace ESM
{
    template <Misc::SameAsWithoutCvref<Apparatus::AADTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mType, v.mQuality, v.mWeight, v.mValue);
    }

    void Apparatus::load(ESMReader& esm, bool& isDeleted)
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
                case fourCC("AADT"):
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
            esm.fail("Missing AADT subrecord");
    }

    void Apparatus::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNCString("FNAM", mName);
        esm.writeNamedComposite("AADT", mData);
        esm.writeHNOCRefId("SCRI", mScript);
        esm.writeHNCString("ITEX", mIcon);
    }

    void Apparatus::blank()
    {
        mRecordFlags = 0;
        mData.mType = 0;
        mData.mQuality = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mModel.clear();
        mIcon.clear();
        mScript = ESM::RefId();
        mName.clear();
    }
}
