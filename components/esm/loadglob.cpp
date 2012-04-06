#include "loadglob.hpp"

namespace ESM
{

void Global::load(ESMReader &esm)
{
    VarType t;
    std::string tmp = esm.getHNString("FNAM");
    if (tmp == "s")
        t = VT_Short;
    else if (tmp == "l")
        t = VT_Int;
    else if (tmp == "f")
        t = VT_Float;
    else
        esm.fail("Illegal global variable type " + tmp);
    type = t;

    // Note: Both floats and longs are represented as floats.
    esm.getHNT(value, "FLTV");
}

void Global::save(ESMWriter &esm)
{
    switch(type)
    {
    case VT_Short: esm.writeHNString("FNAM", "s"); break;
    case VT_Int: esm.writeHNString("FNAM", "l"); break;
    case VT_Float: esm.writeHNString("FNAM", "f"); break;
    default: return;
    }
    
    esm.writeHNT("FLTV", value);
}

}
