#include "variantimp.hpp"

#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    namespace
    {
        template <class T>
        T floatCast(float value)
        {
            // float to int conversions for values outside T's valid range are UB. This code produces a result
            // equivalent to static_cast<T>(value) on x86 without invoking UB.
            constexpr double min = static_cast<double>(std::numeric_limits<int32_t>::lowest());
            constexpr double max = static_cast<double>(std::numeric_limits<int32_t>::max());
            if (std::isnan(value) || value < min || value > max)
                return static_cast<T>(std::numeric_limits<int32_t>::lowest());
            return static_cast<T>(static_cast<int32_t>(value));
        }
    }

    void readESMVariantValue(ESMReader& esm, Variant::Format format, VarType type, std::string& out)
    {
        if (type != VT_String)
            throw std::logic_error("not a string type");

        if (format == Variant::Format_Global)
            esm.fail("global variables of type string not supported");

        if (format == Variant::Format_Info)
            esm.fail("info variables of type string not supported");

        if (format == Variant::Format_Local)
            esm.fail("local variables of type string not supported");

        // GMST
        out = esm.getHString();
    }

    void writeESMVariantValue(ESMWriter& esm, Variant::Format format, VarType type, const std::string& in)
    {
        if (type != VT_String)
            throw std::logic_error("not a string type");

        if (format == Variant::Format_Global)
            throw std::runtime_error("global variables of type string not supported");

        if (format == Variant::Format_Info)
            throw std::runtime_error("info variables of type string not supported");

        if (format == Variant::Format_Local)
            throw std::runtime_error("local variables of type string not supported");

        // GMST
        esm.writeHNString("STRV", in);
    }

    void readESMVariantValue(ESMReader& esm, Variant::Format format, VarType type, int32_t& out)
    {
        if (type != VT_Short && type != VT_Long && type != VT_Int)
            throw std::logic_error("not an integer type");

        if (format == Variant::Format_Global)
        {
            float value;
            esm.getHNT(value, "FLTV");

            if (type == VT_Short)
                out = floatCast<int16_t>(value);
            else if (type == VT_Long)
                out = floatCast<int32_t>(value);
            else
                esm.fail("unsupported global variable integer type");
        }
        else if (format == Variant::Format_Gmst || format == Variant::Format_Info)
        {
            if (type != VT_Int)
            {
                std::ostringstream stream;
                stream << "unsupported " << (format == Variant::Format_Gmst ? "gmst" : "info")
                       << " variable integer type";
                esm.fail(stream.str());
            }

            esm.getHT(out);
        }
        else if (format == Variant::Format_Local)
        {
            if (type == VT_Short)
            {
                int16_t value;
                esm.getHT(value);
                out = value;
            }
            else if (type == VT_Int)
            {
                esm.getHT(out);
            }
            else
                esm.fail("unsupported local variable integer type");
        }
    }

    void writeESMVariantValue(ESMWriter& esm, Variant::Format format, VarType type, int32_t in)
    {
        if (type != VT_Short && type != VT_Long && type != VT_Int)
            throw std::logic_error("not an integer type");

        if (format == Variant::Format_Global)
        {
            if (type == VT_Short || type == VT_Long)
            {
                float value = static_cast<float>(in);
                esm.writeHNString("FNAM", type == VT_Short ? "s" : "l");
                esm.writeHNT("FLTV", value);
            }
            else
                throw std::runtime_error("unsupported global variable integer type");
        }
        else if (format == Variant::Format_Gmst || format == Variant::Format_Info)
        {
            if (type != VT_Int)
            {
                std::ostringstream stream;
                stream << "unsupported " << (format == Variant::Format_Gmst ? "gmst" : "info")
                       << " variable integer type";
                throw std::runtime_error(stream.str());
            }

            esm.writeHNT("INTV", in);
        }
        else if (format == Variant::Format_Local)
        {
            if (type == VT_Short)
                esm.writeHNT("STTV", static_cast<int16_t>(in));
            else if (type == VT_Int)
                esm.writeHNT("INTV", in);
            else
                throw std::runtime_error("unsupported local variable integer type");
        }
    }

    void readESMVariantValue(ESMReader& esm, Variant::Format format, VarType type, float& out)
    {
        if (type != VT_Float)
            throw std::logic_error("not a float type");

        if (format == Variant::Format_Global)
        {
            esm.getHNT(out, "FLTV");
        }
        else if (format == Variant::Format_Gmst || format == Variant::Format_Info || format == Variant::Format_Local)
        {
            esm.getHT(out);
        }
    }

    void writeESMVariantValue(ESMWriter& esm, Variant::Format format, VarType type, float in)
    {
        if (type != VT_Float)
            throw std::logic_error("not a float type");

        if (format == Variant::Format_Global)
        {
            esm.writeHNString("FNAM", "f");
            esm.writeHNT("FLTV", in);
        }
        else if (format == Variant::Format_Gmst || format == Variant::Format_Info || format == Variant::Format_Local)
        {
            esm.writeHNT("FLTV", in);
        }
    }

}
