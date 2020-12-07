#include "variant.hpp"

#include <cassert>
#include <stdexcept>

#include "esmreader.hpp"
#include "variantimp.hpp"

#include "defs.hpp"

namespace
{
    const uint32_t STRV = ESM::FourCC<'S','T','R','V'>::value;
    const uint32_t INTV = ESM::FourCC<'I','N','T','V'>::value;
    const uint32_t FLTV = ESM::FourCC<'F','L','T','V'>::value;
    const uint32_t STTV = ESM::FourCC<'S','T','T','V'>::value;
}

ESM::Variant::Variant() : mType (VT_None), mData (nullptr) {}

ESM::Variant::Variant(const std::string &value)
{
    mData = nullptr;
    mType = VT_None;
    setType(VT_String);
    setString(value);
}

ESM::Variant::Variant(int value)
{
    mData = nullptr;
    mType = VT_None;
    setType(VT_Long);
    setInteger(value);
}

ESM::Variant::Variant(float value)
{
    mData = nullptr;
    mType = VT_None;
    setType(VT_Float);
    setFloat(value);
}

ESM::Variant::~Variant()
{
    delete mData;
}

ESM::Variant& ESM::Variant::operator= (const Variant& variant)
{
    if (&variant!=this)
    {
        VariantDataBase *newData = variant.mData ? variant.mData->clone() : nullptr;

        delete mData;

        mType = variant.mType;
        mData = newData;
    }

    return *this;
}

ESM::Variant::Variant (const Variant& variant)
: mType (variant.mType), mData (variant.mData ? variant.mData->clone() : nullptr)
{}

ESM::VarType ESM::Variant::getType() const
{
    return mType;
}

std::string ESM::Variant::getString() const
{
    if (!mData)
        throw std::runtime_error ("can not convert empty variant to string");

    return mData->getString();
}

int ESM::Variant::getInteger() const
{
    if (!mData)
        throw std::runtime_error ("can not convert empty variant to integer");

    return mData->getInteger();
}

float ESM::Variant::getFloat() const
{
    if (!mData)
        throw std::runtime_error ("can not convert empty variant to float");

    return mData->getFloat();
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

    // data
    if (mData)
        mData->read (esm, format, mType);
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
        mData->write (esm, format, mType);
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

            stream << "variant short: " << mData->getInteger();
            break;

        case VT_Int:

            stream << "variant int: " << mData->getInteger();
            break;

        case VT_Long:

            stream << "variant long: " << mData->getInteger();
            break;

        case VT_Float:

            stream << "variant float: " << mData->getFloat();
            break;

        case VT_String:

            stream << "variant string: \"" << mData->getString() << "\"";
            break;
    }
}

void ESM::Variant::setType (VarType type)
{
    if (type!=mType)
    {
        VariantDataBase *newData = nullptr;

        switch (type)
        {
            case VT_Unknown:
            case VT_None:

                break; // no data

            case VT_Short:
            case VT_Int:
            case VT_Long:

                newData = new VariantIntegerData (mData);
                break;

            case VT_Float:

                newData = new VariantFloatData (mData);
                break;

            case VT_String:

                newData = new VariantStringData (mData);
                break;
        }

        delete mData;
        mData = newData;
        mType = type;
    }
}

void ESM::Variant::setString (const std::string& value)
{
    if (!mData)
        throw std::runtime_error ("can not assign string to empty variant");

    mData->setString (value);
}

void ESM::Variant::setInteger (int value)
{
    if (!mData)
        throw std::runtime_error ("can not assign integer to empty variant");

    mData->setInteger (value);
}

void ESM::Variant::setFloat (float value)
{
    if (!mData)
        throw std::runtime_error ("can not assign float to empty variant");

    mData->setFloat (value);
}

bool ESM::Variant::isEqual (const Variant& value) const
{
    if (mType!=value.mType)
        return false;

    if (!mData)
        return true;

    assert (value.mData);

    return mData->isEqual (*value.mData);
}

std::ostream& ESM::operator<< (std::ostream& stream, const Variant& value)
{
    value.write (stream);
    return stream;
}

bool ESM::operator== (const Variant& left, const Variant& right)
{
    return left.isEqual (right);
}

bool ESM::operator!= (const Variant& left, const Variant& right)
{
    return !(left==right);
}
