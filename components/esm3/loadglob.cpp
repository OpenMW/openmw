#include "loadglob.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
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
        mRecordFlags = 0;
        mValue.setType (VT_None);
    }

    bool operator== (const Global& left, const Global& right)
    {
        return left.mId==right.mId && left.mValue==right.mValue;
    }
}
