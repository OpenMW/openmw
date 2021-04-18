#include "variantimp.hpp"

#include <stdexcept>
#include <cmath>

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::readESMVariantValue(ESMReader& esm, Variant::Format format, VarType type, std::string& out)
{
    if (type!=VT_String)
        throw std::logic_error ("not a string type");

    if (format==Variant::Format_Global)
        esm.fail ("global variables of type string not supported");

    if (format==Variant::Format_Info)
        esm.fail ("info variables of type string not supported");

    if (format==Variant::Format_Local)
        esm.fail ("local variables of type string not supported");

    // GMST
    out = esm.getHString();
}

void ESM::writeESMVariantValue(ESMWriter& esm, Variant::Format format, VarType type, const std::string& in)
{
    if (type!=VT_String)
        throw std::logic_error ("not a string type");

    if (format==Variant::Format_Global)
        throw std::runtime_error ("global variables of type string not supported");

    if (format==Variant::Format_Info)
        throw std::runtime_error ("info variables of type string not supported");

    if (format==Variant::Format_Local)
        throw std::runtime_error ("local variables of type string not supported");

    // GMST
    esm.writeHNString("STRV", in);
}

void ESM::readESMVariantValue(ESMReader& esm, Variant::Format format, VarType type, int& out)
{
    if (type!=VT_Short && type!=VT_Long && type!=VT_Int)
        throw std::logic_error ("not an integer type");

    if (format==Variant::Format_Global)
    {
        float value;
        esm.getHNT (value, "FLTV");

        if (type==VT_Short)
            if (std::isnan(value))
                out = 0;
            else
                out = static_cast<short> (value);
        else if (type==VT_Long)
            out = static_cast<int> (value);
        else
            esm.fail ("unsupported global variable integer type");
    }
    else if (format==Variant::Format_Gmst || format==Variant::Format_Info)
    {
        if (type!=VT_Int)
        {
            std::ostringstream stream;
            stream
                << "unsupported " <<(format==Variant::Format_Gmst ? "gmst" : "info")
                << " variable integer type";
            esm.fail (stream.str());
        }

        esm.getHT(out);
    }
    else if (format==Variant::Format_Local)
    {
        if (type==VT_Short)
        {
            short value;
            esm.getHT(value);
            out = value;
        }
        else if (type==VT_Int)
        {
            esm.getHT(out);
        }
        else
            esm.fail("unsupported local variable integer type");
    }
}

void ESM::writeESMVariantValue(ESMWriter& esm, Variant::Format format, VarType type, int in)
{
    if (type!=VT_Short && type!=VT_Long && type!=VT_Int)
        throw std::logic_error ("not an integer type");

    if (format==Variant::Format_Global)
    {
        if (type==VT_Short || type==VT_Long)
        {
            float value = static_cast<float>(in);
            esm.writeHNString ("FNAM", type==VT_Short ? "s" : "l");
            esm.writeHNT ("FLTV", value);
        }
        else
            throw std::runtime_error ("unsupported global variable integer type");
    }
    else if (format==Variant::Format_Gmst || format==Variant::Format_Info)
    {
        if (type!=VT_Int)
        {
            std::ostringstream stream;
            stream
                << "unsupported " <<(format==Variant::Format_Gmst ? "gmst" : "info")
                << " variable integer type";
            throw std::runtime_error (stream.str());
        }

        esm.writeHNT("INTV", in);
    }
    else if (format==Variant::Format_Local)
    {
        if (type==VT_Short)
            esm.writeHNT("STTV", static_cast<short>(in));
        else if (type == VT_Int)
            esm.writeHNT("INTV", in);
        else
            throw std::runtime_error("unsupported local variable integer type");
    }
}

void ESM::readESMVariantValue(ESMReader& esm, Variant::Format format, VarType type, float& out)
{
    if (type!=VT_Float)
        throw std::logic_error ("not a float type");

    if (format==Variant::Format_Global)
    {
        esm.getHNT(out, "FLTV");
    }
    else if (format==Variant::Format_Gmst || format==Variant::Format_Info || format==Variant::Format_Local)
    {
        esm.getHT(out);
    }
}

void ESM::writeESMVariantValue(ESMWriter& esm, Variant::Format format, VarType type, float in)
{
    if (type!=VT_Float)
        throw std::logic_error ("not a float type");

    if (format==Variant::Format_Global)
    {
        esm.writeHNString ("FNAM", "f");
        esm.writeHNT("FLTV", in);
    }
    else if (format==Variant::Format_Gmst || format==Variant::Format_Info || format==Variant::Format_Local)
    {
        esm.writeHNT("FLTV", in);
    }
}
