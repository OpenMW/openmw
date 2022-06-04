#include "loadgmst.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    void GameSetting::load (ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false; // GameSetting record can't be deleted now (may be changed in the future)
        mRecordFlags = esm.getRecordFlags();

        mId = esm.getHNString("NAME");
        mValue.read (esm, Variant::Format_Gmst);
    }

    void GameSetting::save (ESMWriter &esm, bool /*isDeleted*/) const
    {
        esm.writeHNCString("NAME", mId);
        mValue.write (esm, Variant::Format_Gmst);
    }

    void GameSetting::blank()
    {
        mRecordFlags = 0;
        mValue.setType (VT_None);
    }

    bool operator== (const GameSetting& left, const GameSetting& right)
    {
        return left.mValue==right.mValue;
    }
}
