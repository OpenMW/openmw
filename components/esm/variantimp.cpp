
#include "variantimp.hpp"

#include <stdexcept>

#include "esmreader.hpp"
#include "esmwriter.hpp"

ESM::VariantDataBase::~VariantDataBase() {}

std::string ESM::VariantDataBase::getString (bool default_) const
{
    if (default_)
        return "";

    throw std::runtime_error ("can not convert variant to string");
}

int ESM::VariantDataBase::getInteger (bool default_) const
{
    if (default_)
        return 0;

    throw std::runtime_error ("can not convert variant to integer");
}

float ESM::VariantDataBase::getFloat (bool default_) const
{
    if (default_)
        return 0;

    throw std::runtime_error ("can not convert variant to float");
}

void ESM::VariantDataBase::setString (const std::string& value)
{
    throw std::runtime_error ("conversion of string to variant not possible");
}

void ESM::VariantDataBase::setInteger (int value)
{
    throw std::runtime_error ("conversion of integer to variant not possible");
}

void ESM::VariantDataBase::setFloat (float value)
{
    throw std::runtime_error ("conversion of float to variant not possible");
}



ESM::VariantStringData::VariantStringData (const VariantDataBase *data)
{
    if (data)
        mValue = data->getString (true);
}

ESM::VariantDataBase *ESM::VariantStringData::clone() const
{
    return new VariantStringData (*this);
}

std::string ESM::VariantStringData::getString (bool default_) const
{
    return mValue;
}

void ESM::VariantStringData::setString (const std::string& value)
{
    mValue = value;
}

void ESM::VariantStringData::read (ESMReader& esm, Variant::Format format, VarType type)
{
    if (type!=VT_String)
        throw std::logic_error ("not a string type");

    if (format==Variant::Format_Global)
        esm.fail ("global variables of type string not supported");

    if (format==Variant::Format_Info)
        esm.fail ("info variables of type string not supported");

    // GMST
    mValue = esm.getHString();
}

void ESM::VariantStringData::write (ESMWriter& esm, Variant::Format format, VarType type) const
{
    if (type!=VT_String)
        throw std::logic_error ("not a string type");

    if (format==Variant::Format_Global)
        throw std::runtime_error ("global variables of type string not supported");

    if (format==Variant::Format_Info)
        throw std::runtime_error ("info variables of type string not supported");

    // GMST
    esm.writeHNString ("STRV", mValue);
}

bool ESM::VariantStringData::isEqual (const VariantDataBase& value) const
{
    return dynamic_cast<const VariantStringData&> (value).mValue==mValue;
}



ESM::VariantIntegerData::VariantIntegerData (const VariantDataBase *data) : mValue (0)
{
    if (data)
        mValue = data->getInteger (true);
}

ESM::VariantDataBase *ESM::VariantIntegerData::clone() const
{
    return new VariantIntegerData (*this);
}

int ESM::VariantIntegerData::getInteger (bool default_) const
{
    return mValue;
}

float ESM::VariantIntegerData::getFloat (bool default_) const
{
    return mValue;
}

void ESM::VariantIntegerData::setInteger (int value)
{
    mValue = value;
}

void ESM::VariantIntegerData::setFloat (float value)
{
    mValue = static_cast<int> (value);
}

void ESM::VariantIntegerData::read (ESMReader& esm, Variant::Format format, VarType type)
{
    if (type!=VT_Short && type!=VT_Long && type!=VT_Int)
        throw std::logic_error ("not an integer type");

    if (format==Variant::Format_Global)
    {
        float value;
        esm.getHNT (value, "FLTV");

        if (type==VT_Short)
        {
            if (value!=value)
                mValue = 0; // nan
            else
                mValue = static_cast<short> (value);
        }
        else if (type==VT_Long)
            mValue = static_cast<int> (value);
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

        esm.getHT (mValue);
    }
}

void ESM::VariantIntegerData::write (ESMWriter& esm, Variant::Format format, VarType type) const
{
    if (type!=VT_Short && type!=VT_Long && type!=VT_Int)
        throw std::logic_error ("not an integer type");

    if (format==Variant::Format_Global)
    {
        if (type==VT_Short || type==VT_Long)
        {
            float value = mValue;
            esm.writeHNString ("FNAM", type==VT_Short ? "s" : "l");
            esm.writeHNT ("FLTV", value);
        }
        else
            throw std::runtime_error ("unsupported global variable integer type");
    }
    else if (format==Variant::Format_Gmst || format==Variant::Format_Info)
    {
        if (type==VT_Int)
        {
            std::ostringstream stream;
            stream
                << "unsupported " <<(format==Variant::Format_Gmst ? "gmst" : "info")
                << " variable integer type";
            throw std::runtime_error (stream.str());
        }

        esm.writeHNT ("INTV", mValue);
    }
}

bool ESM::VariantIntegerData::isEqual (const VariantDataBase& value) const
{
    return dynamic_cast<const VariantIntegerData&> (value).mValue==mValue;
}


ESM::VariantFloatData::VariantFloatData (const VariantDataBase *data) : mValue (0)
{
    if (data)
        mValue = data->getFloat (true);
}

ESM::VariantDataBase *ESM::VariantFloatData::clone() const
{
    return new VariantFloatData (*this);
}

int ESM::VariantFloatData::getInteger (bool default_) const
{
    return static_cast<int> (mValue);
}

float ESM::VariantFloatData::getFloat (bool default_) const
{
    return mValue;
}

void ESM::VariantFloatData::setInteger (int value)
{
    mValue = value;
}

void ESM::VariantFloatData::setFloat (float value)
{
    mValue = value;
}

void ESM::VariantFloatData::read (ESMReader& esm, Variant::Format format, VarType type)
{
    if (type!=VT_Float)
        throw std::logic_error ("not a float type");

    if (format==Variant::Format_Global)
    {
        esm.getHNT (mValue, "FLTV");
    }
    else if (format==Variant::Format_Gmst || format==Variant::Format_Info)
    {
        esm.getHT (mValue);
    }
}

void ESM::VariantFloatData::write (ESMWriter& esm, Variant::Format format, VarType type) const
{
    if (type!=VT_Float)
        throw std::logic_error ("not a float type");

    if (format==Variant::Format_Global)
    {
        esm.writeHNString ("FNAM", "f");
        esm.writeHNT ("FLTV", mValue);
    }
    else if (format==Variant::Format_Gmst || format==Variant::Format_Info)
    {
        esm.writeHNT ("FLTV", mValue);
    }
}

bool ESM::VariantFloatData::isEqual (const VariantDataBase& value) const
{
    return dynamic_cast<const VariantFloatData&> (value).mValue==mValue;
}