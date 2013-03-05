#ifndef OPENMW_ESM_VARIANT_H
#define OPENMW_ESM_VARIANT_H

#include <string>
#include <iosfwd>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    enum VarType
    {
        VT_Unknown,
        VT_None,
        VT_Short, // stored as a float, kinda
        VT_Int,
        VT_Long, // stored as a float
        VT_Float,
        VT_String
    };

    class VariantDataBase;

    class Variant
    {
            VarType mType;
            VariantDataBase *mData;

        public:

            enum Format
            {
                Format_Global,
                Format_Gmst,
                Format_Info
            };

            Variant();

            ~Variant();

            Variant& operator= (const Variant& variant);

            Variant (const Variant& variant);

            VarType getType() const;

            std::string getString() const;
            ///< Will throw an exception, if value can not be represented as a string.

            int getInteger() const;
            ///< Will throw an exception, if value can not be represented as an integer (implicit
            /// casting of float values is permitted).

            float getFloat() const;
            ///< Will throw an exception, if value can not be represented as a float value.

            void read (ESMReader& esm, Format format);

            void write (ESMWriter& esm, Format format) const;

            void write (std::ostream& stream) const;
            ///< Write in text format.

            void setType (VarType type);

            void setString (const std::string& value);
            ///< Will throw an exception, if type is not compatible with string.

            void setInteger (int value);
            ///< Will throw an exception, if type is not compatible with integer.

            void setFloat (float value);
            ///< Will throw an exception, if type is not compatible with float.

            bool isEqual (const Variant& value) const;
    };

    std::ostream& operator<<(std::ostream& stream, const Variant& value);

    bool operator== (const Variant& left, const Variant& right);
    bool operator!= (const Variant& left, const Variant& right);
}

#endif