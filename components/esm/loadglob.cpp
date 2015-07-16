#include "loadglob.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Global::sRecordId = REC_GLOB;

    Global::Global()
        : mIsDeleted(false)
    {}

    void Global::load (ESMReader &esm)
    {
        mIsDeleted = false;
        mId = esm.getHNString ("NAME");

        if (esm.isNextSub ("DELE"))
        {
            esm.skipHSub();
            mIsDeleted = true;
        }
        else
        {
            mValue.read (esm, ESM::Variant::Format_Global);
        }
    }

    void Global::save (ESMWriter &esm) const
    {
        esm.writeHNCString ("NAME", mId);

        if (mIsDeleted)
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
        mIsDeleted = false;
    }

    bool operator== (const Global& left, const Global& right)
    {
        return left.mId==right.mId && left.mValue==right.mValue;
    }
}
