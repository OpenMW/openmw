#include "loadalch.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/concepts.hpp>

namespace ESM
{
    template <Misc::SameAsWithoutCvref<Potion::ALDTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mWeight, v.mValue, v.mFlags);
    }

    void Potion::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mEffects.mList.clear();

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
                case fourCC("TEXT"): // not ITEX here for some reason
                    mIcon = esm.getHString();
                    break;
                case fourCC("SCRI"):
                    mScript = esm.getRefId();
                    break;
                case fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case fourCC("ALDT"):
                    esm.getSubComposite(mData);
                    hasData = true;
                    break;
                case fourCC("ENAM"):
                    mEffects.add(esm);
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
            esm.fail("Missing ALDT subrecord");
    }
    void Potion::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("TEXT", mIcon);
        esm.writeHNOCRefId("SCRI", mScript);
        esm.writeHNOCString("FNAM", mName);
        esm.writeNamedComposite("ALDT", mData);
        mEffects.save(esm);
    }

    void Potion::blank()
    {
        mRecordFlags = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mFlags = 0;
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript = ESM::RefId();
        mEffects.mList.clear();
    }
}
