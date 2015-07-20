#include "loadglob.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Global::sRecordId = REC_GLOB;

    void Global::load (ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mId = esm.getHNString ("NAME");

        if (esm.isNextSub ("DELE"))
        {
            esm.skipHSub();
            isDeleted = true;
        }
        else
        {
            mValue.read (esm, ESM::Variant::Format_Global);
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
            mValue.write (esm, ESM::Variant::Format_Global);
        }
    }

    void Global::blank()
    {
        mValue.setType (ESM::VT_None);
    }

    bool operator== (const Global& left, const Global& right)
    {
        return left.mId==right.mId && left.mValue==right.mValue;
    }
}
