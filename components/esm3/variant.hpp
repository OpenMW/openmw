#ifndef OPENMW_ESM_VARIANT_H
#define OPENMW_ESM_VARIANT_H

#include <string>
#include <iosfwd>
#include <variant>
#include <tuple>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    enum VarType
    {
        VT_Unknown = 0,
        VT_None,
        VT_Short, // stored as a float, kinda
        VT_Int,
        VT_Long, // stored as a float
        VT_Float,
        VT_String
    };

    class Variant
    {
            VarType mType;
            std::variant<std::monostate, int, float, std::string> mData;

        public:

            enum Format
            {
                Format_Global,
                Format_Gmst,
                Format_Info,
                Format_Local // local script variables in save game files
            };

            Variant() : mType (VT_None), mData (std::monostate{}) {}

            explicit Variant(const std::string& value) : mType(VT_String), mData(value) {}

            explicit Variant(std::string&& value) : mType(VT_String), mData(std::move(value)) {}

            explicit Variant(int value) : mType(VT_Long), mData(value) {}

            explicit Variant(float value) : mType(VT_Float), mData(value) {}

            VarType getType() const { return mType; }

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

            void setString (std::string&& value);
            ///< Will throw an exception, if type is not compatible with string.

            void setInteger (int value);
            ///< Will throw an exception, if type is not compatible with integer.

            void setFloat (float value);
            ///< Will throw an exception, if type is not compatible with float.

            friend bool operator==(const Variant& left, const Variant& right)
            {
                return std::tie(left.mType, left.mData) == std::tie(right.mType, right.mData);
            }

            friend bool operator!=(const Variant& left, const Variant& right)
            {
                return !(left == right);
            }
    };

    std::ostream& operator<<(std::ostream& stream, const Variant& value);
}

#endif
