#include "loadgmst.hpp"

#include "defs.hpp"

namespace ESM
{
    unsigned int GameSetting::sRecordId = REC_GMST;

    void GameSetting::load (ESMReader &esm)
    {
        mValue.read (esm, ESM::Variant::Format_Gmst);
    }

    void GameSetting::save (ESMWriter &esm) const
    {
        mValue.write (esm, ESM::Variant::Format_Gmst);
    }

    int GameSetting::getInt() const
    {
        return mValue.getInteger();
    }

    float GameSetting::getFloat() const
    {
        return mValue.getFloat();
    }

    std::string GameSetting::getString() const
    {
        return mValue.getString();
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
