#include "loadgmst.hpp"

#include <stdexcept>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void GameSetting::load(ESMReader &esm)
{
    assert(mId != "");

    // We are apparently allowed to be empty
    if (!esm.hasMoreSubs())
    {
        mType = VT_None;
        return;
    }

    // Load some data
    esm.getSubName();
    NAME n = esm.retSubName();
    if (n == "STRV")
    {
        mStr = esm.getHString();
        mType = VT_String;
    }
    else if (n == "INTV")
    {
        esm.getHT(mI);
        mType = VT_Int;
    }
    else if (n == "FLTV")
    {
        esm.getHT(mF);
        mType = VT_Float;
    }
    else
        esm.fail("Unwanted subrecord type");
}

void GameSetting::save(ESMWriter &esm)
{
    switch(mType)
    {
    case VT_String: esm.writeHNString("STRV", mStr); break;
    case VT_Int: esm.writeHNT("INTV", mI); break;
    case VT_Float: esm.writeHNT("FLTV", mF); break;
    default: break;
    }
}

int GameSetting::getInt() const
{
    switch (mType)
    {
        case VT_Float: return static_cast<int> (mF);
        case VT_Int: return mI;
        default: throw std::runtime_error ("GMST " + mId + " is not of a numeric type");
    }
}

float GameSetting::getFloat() const
{
    switch (mType)
    {
        case VT_Float: return mF;
        case VT_Int: return mI;
        default: throw std::runtime_error ("GMST " + mId + " is not of a numeric type");
    }
}

std::string GameSetting::getString() const
{
    if (mType==VT_String)
        return mStr;
        
    throw std::runtime_error ("GMST " + mId + " is not a string");
}

}
