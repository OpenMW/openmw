#include "loadglob.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "components/esm/defs.hpp"

namespace ESM
{
    unsigned int Global::sRecordId = REC_GLOB;

    void Global::load (ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mId = esm.getHNString ("NAME");

        if (esm.isNextSub ("DELE"))
        {
            esm.skipHSub();
            isDeleted = true;
        }
        else
        {
            mValue.read (esm, Variant::Format_Global);
        }
    }

    void Global::save (ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString ("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString ("DELE", "");
        }
        else
        {
            mValue.write (esm, Variant::Format_Global);
        }
    }

    void Global::blank()
    {
        mValue.setType (VT_None);
    }

    bool operator== (const Global& left, const Global& right)
    {
        return left.mId==right.mId && left.mValue==right.mValue;
    }
}
