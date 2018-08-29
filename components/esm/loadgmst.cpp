#include "loadgmst.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int GameSetting::sRecordId = REC_GMST;

    void GameSetting::load (ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false; // GameSetting record can't be deleted now (may be changed in the future)

        mId = esm.getHNString("NAME");
        mValue.read (esm, ESM::Variant::Format_Gmst);
    }

    void GameSetting::save (ESMWriter &esm, bool /*isDeleted*/) const
    {
        esm.writeHNCString("NAME", mId);
        mValue.write (esm, ESM::Variant::Format_Gmst);
    }

    void GameSetting::blank()
    {
        mValue.setType (ESM::VT_None);
    }

    bool operator== (const GameSetting& left, const GameSetting& right)
    {
        return left.mValue==right.mValue;
    }
}
