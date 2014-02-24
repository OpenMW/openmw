#ifndef OPENMW_ESM_VARIANTIMP_H
#define OPENMW_ESM_VARIANTIMP_H

#include <string>

#include "variant.hpp"

namespace ESM
{
    class VariantDataBase
    {
        public:

            virtual ~VariantDataBase();

            virtual VariantDataBase *clone() const = 0;

            virtual std::string getString (bool default_ = false) const;
            ///< Will throw an exception, if value can not be represented as a string.
            ///
            /// \note Numeric values are not converted to strings.
            ///
            /// \param default_ Return a default value instead of throwing an exception.
            ///
            /// Default-implementation: throw an exception.

            virtual int getInteger (bool default_ = false) const;
            ///< Will throw an exception, if value can not be represented as an integer (implicit
            /// casting of float values is permitted).
            ///
            /// \param default_ Return a default value instead of throwing an exception.
            ///
            /// Default-implementation: throw an exception.

            virtual float getFloat (bool default_ = false) const;
            ///< Will throw an exception, if value can not be represented as a float value.
            ///
            /// \param default_ Return a default value instead of throwing an exception.
            ///
            /// Default-implementation: throw an exception.

            virtual void setString (const std::string& value);
            ///< Will throw an exception, if type is not compatible with string.
            ///
            /// Default-implementation: throw an exception.

            virtual void setInteger (int value);
            ///< Will throw an exception, if type is not compatible with integer.
            ///
            /// Default-implementation: throw an exception.

            virtual void setFloat (float value);
            ///< Will throw an exception, if type is not compatible with float.
            ///
            /// Default-implementation: throw an exception.

            virtual void read (ESMReader& esm, Variant::Format format, VarType type) = 0;
            ///< If \a type is not supported by \a format, an exception is thrown via ESMReader::fail

            virtual void write (ESMWriter& esm, Variant::Format format, VarType type) const = 0;
            ///< If \a type is not supported by \a format, an exception is thrown.

            virtual bool isEqual (const VariantDataBase& value) const = 0;
            ///< If the (C++) type of \a value does not match the type of *this, an exception is thrown.

    };

    class VariantStringData : public VariantDataBase
    {
            std::string mValue;

        public:

            VariantStringData (const VariantDataBase *data = 0);
            ///< Calling the constructor with an incompatible data type will result in a silent
            /// default initialisation.

            virtual VariantDataBase *clone() const;

            virtual std::string getString (bool default_ = false) const;
            ///< Will throw an exception, if value can not be represented as a string.
            ///
            /// \note Numeric values are not converted to strings.
            ///
            /// \param default_ Return a default value instead of throwing an exception.

            virtual void setString (const std::string& value);
            ///< Will throw an exception, if type is not compatible with string.

            virtual void read (ESMReader& esm, Variant::Format format, VarType type);
            ///< If \a type is not supported by \a format, an exception is thrown via ESMReader::fail

            virtual void write (ESMWriter& esm, Variant::Format format, VarType type) const;
            ///< If \a type is not supported by \a format, an exception is thrown.

            virtual bool isEqual (const VariantDataBase& value) const;
            ///< If the (C++) type of \a value does not match the type of *this, an exception is thrown.
    };

    class VariantIntegerData : public VariantDataBase
    {
            int mValue;

        public:

            VariantIntegerData (const VariantDataBase *data = 0);
            ///< Calling the constructor with an incompatible data type will result in a silent
            /// default initialisation.

            virtual VariantDataBase *clone() const;

            virtual int getInteger (bool default_ = false) const;
            ///< Will throw an exception, if value can not be represented as an integer (implicit
            /// casting of float values is permitted).
            ///
            /// \param default_ Return a default value instead of throwing an exception.

            virtual float getFloat (bool default_ = false) const;
            ///< Will throw an exception, if value can not be represented as a float value.
            ///
            /// \param default_ Return a default value instead of throwing an exception.

            virtual void setInteger (int value);
            ///< Will throw an exception, if type is not compatible with integer.

            virtual void setFloat (float value);
            ///< Will throw an exception, if type is not compatible with float.

            virtual void read (ESMReader& esm, Variant::Format format, VarType type);
            ///< If \a type is not supported by \a format, an exception is thrown via ESMReader::fail

            virtual void write (ESMWriter& esm, Variant::Format format, VarType type) const;
            ///< If \a type is not supported by \a format, an exception is thrown.

            virtual bool isEqual (const VariantDataBase& value) const;
            ///< If the (C++) type of \a value does not match the type of *this, an exception is thrown.
    };

    class VariantFloatData : public VariantDataBase
    {
            float mValue;

        public:

            VariantFloatData (const VariantDataBase *data = 0);
            ///< Calling the constructor with an incompatible data type will result in a silent
            /// default initialisation.

            virtual VariantDataBase *clone() const;

            virtual int getInteger (bool default_ = false) const;
            ///< Will throw an exception, if value can not be represented as an integer (implicit
            /// casting of float values is permitted).
            ///
            /// \param default_ Return a default value instead of throwing an exception.

            virtual float getFloat (bool default_ = false) const;
            ///< Will throw an exception, if value can not be represented as a float value.
            ///
            /// \param default_ Return a default value instead of throwing an exception.

            virtual void setInteger (int value);
            ///< Will throw an exception, if type is not compatible with integer.

            virtual void setFloat (float value);
            ///< Will throw an exception, if type is not compatible with float.

            virtual void read (ESMReader& esm, Variant::Format format, VarType type);
            ///< If \a type is not supported by \a format, an exception is thrown via ESMReader::fail

            virtual void write (ESMWriter& esm, Variant::Format format, VarType type) const;
            ///< If \a type is not supported by \a format, an exception is thrown.

            virtual bool isEqual (const VariantDataBase& value) const;
            ///< If the (C++) type of \a value does not match the type of *this, an exception is thrown.
    };
}

#endif
