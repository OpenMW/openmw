#include "loadglob.hpp"

#include "defs.hpp"

namespace ESM
{
    unsigned int Global::sRecordId = REC_GLOB;

    void Global::load (ESMReader &esm)
    {
        mValue.read (esm, ESM::Variant::Format_Global);
    }

    void Global::save (ESMWriter &esm) const
    {
        mValue.write (esm, ESM::Variant::Format_Global);
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
