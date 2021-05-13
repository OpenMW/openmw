#include "variant.hpp"

#include <cassert>
#include <stdexcept>

#include "esmreader.hpp"
#include "variantimp.hpp"

#include "defs.hpp"

namespace
{
    constexpr uint32_t STRV = ESM::FourCC<'S','T','R','V'>::value;
    constexpr uint32_t INTV = ESM::FourCC<'I','N','T','V'>::value;
    constexpr uint32_t FLTV = ESM::FourCC<'F','L','T','V'>::value;
    constexpr uint32_t STTV = ESM::FourCC<'S','T','T','V'>::value;

    template <typename T, bool orDefault = false>
    struct GetValue
    {
        constexpr T operator()(int value) const { return static_cast<T>(value); }

        constexpr T operator()(float value) const { return static_cast<T>(value); }

        template <typename V>
        constexpr T operator()(const V&) const
        {
            if constexpr (orDefault)
                return T {};
            else
                throw std::runtime_error("cannot convert variant");
        }
    };

    template <typename T>
    struct SetValue
    {
        T mValue;

        explicit SetValue(T value) : mValue(value) {}

        void operator()(int& value) const { value = static_cast<int>(mValue); }

        void operator()(float& value) const { value = static_cast<float>(mValue); }

        template <typename V>
        void operator()(V&) const { throw std::runtime_error("cannot convert variant"); }
    };
}

std::string ESM::Variant::getString() const
{
    return std::get<std::string>(mData);
}

int ESM::Variant::getInteger() const
{
    return std::visit(GetValue<int>{}, mData);
}

float ESM::Variant::getFloat() const
{
    return std::visit(GetValue<float>{}, mData);
}

void ESM::Variant::read (ESMReader& esm, Format format)
{
    // type
    VarType type = VT_Unknown;

    if (format==Format_Global)
    {
        std::string typeId = esm.getHNString ("FNAM");

        if (typeId == "s")
            type = VT_Short;
        else if (typeId == "l")
            type = VT_Long;
        else if (typeId == "f")
            type = VT_Float;
        else
            esm.fail ("illegal global variable type " + typeId);
    }
    else if (format==Format_Gmst)
    {
        if (!esm.hasMoreSubs())
        {
            type = VT_None;
        }
        else
        {
            esm.getSubName();
            NAME name = esm.retSubName();



            if (name==STRV)
            {
                type = VT_String;
            }
            else if (name==INTV)
            {
                type = VT_Int;
            }
            else if (name==FLTV)
            {
                type = VT_Float;
            }
            else
                esm.fail ("invalid subrecord: " + name.toString());
        }
    }
    else if (format == Format_Info)
    {
        esm.getSubName();
        NAME name = esm.retSubName();

        if (name==INTV)
        {
            type = VT_Int;
        }
        else if (name==FLTV)
        {
            type = VT_Float;
        }
        else
            esm.fail ("invalid subrecord: " + name.toString());
    }
    else if (format == Format_Local)
    {
        esm.getSubName();
        NAME name = esm.retSubName();

        if (name==INTV)
        {
            type = VT_Int;
        }
        else if (name==FLTV)
        {
            type = VT_Float;
        }
        else if (name==STTV)
        {
            type = VT_Short;
        }
        else
            esm.fail ("invalid subrecord: " + name.toString());
    }

    setType (type);

    std::visit(ReadESMVariantValue {esm, format, mType}, mData);
}

void ESM::Variant::write (ESMWriter& esm, Format format) const
{
    if (mType==VT_Unknown)
    {
        throw std::runtime_error ("can not serialise variant of unknown type");
    }
    else if (mType==VT_None)
    {
        if (format==Format_Global)
            throw std::runtime_error ("can not serialise variant of type none to global format");

        if (format==Format_Info)
            throw std::runtime_error ("can not serialise variant of type none to info format");

        if (format==Format_Local)
            throw std::runtime_error ("can not serialise variant of type none to local format");

        // nothing to do here for GMST format
    }
    else
        std::visit(WriteESMVariantValue {esm, format, mType}, mData);
}

void ESM::Variant::write (std::ostream& stream) const
{
    switch (mType)
    {
        case VT_Unknown:

            stream << "variant unknown";
            break;

        case VT_None:

            stream << "variant none";
            break;

        case VT_Short:

            stream << "variant short: " << std::get<int>(mData);
            break;

        case VT_Int:

            stream << "variant int: " << std::get<int>(mData);
            break;

        case VT_Long:

            stream << "variant long: " << std::get<int>(mData);
            break;

        case VT_Float:

            stream << "variant float: " << std::get<float>(mData);
            break;

        case VT_String:

            stream << "variant string: \"" << std::get<std::string>(mData) << "\"";
            break;
    }
}

void ESM::Variant::setType (VarType type)
{
    if (type!=mType)
    {
        switch (type)
        {
            case VT_Unknown:
            case VT_None:
                mData = std::monostate {};
                break;

            case VT_Short:
            case VT_Int:
            case VT_Long:
                mData = std::visit(GetValue<int, true>{}, mData);
                break;

            case VT_Float:
                mData = std::visit(GetValue<float, true>{}, mData);
                break;

            case VT_String:
                mData = std::string {};
                break;
        }

        mType = type;
    }
}

void ESM::Variant::setString (const std::string& value)
{
    std::get<std::string>(mData) = value;
}

void ESM::Variant::setString (std::string&& value)
{
    std::get<std::string>(mData) = std::move(value);
}

void ESM::Variant::setInteger (int value)
{
    std::visit(SetValue(value), mData);
}

void ESM::Variant::setFloat (float value)
{
    std::visit(SetValue(value), mData);
}

std::ostream& ESM::operator<< (std::ostream& stream, const Variant& value)
{
    value.write (stream);
    return stream;
}
