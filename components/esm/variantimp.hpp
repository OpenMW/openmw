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

            VariantStringData (const VariantDataBase *data = nullptr);
            ///< Calling the constructor with an incompatible data type will result in a silent
            /// default initialisation.

            VariantDataBase *clone() const override;

            std::string getString (bool default_ = false) const override;
            ///< Will throw an exception, if value can not be represented as a string.
            ///
            /// \note Numeric values are not converted to strings.
            ///
            /// \param default_ Return a default value instead of throwing an exception.

            void setString (const std::string& value) override;
            ///< Will throw an exception, if type is not compatible with string.

            void read (ESMReader& esm, Variant::Format format, VarType type) override;
            ///< If \a type is not supported by \a format, an exception is thrown via ESMReader::fail

            void write (ESMWriter& esm, Variant::Format format, VarType type) const override;
            ///< If \a type is not supported by \a format, an exception is thrown.

            bool isEqual (const VariantDataBase& value) const override;
            ///< If the (C++) type of \a value does not match the type of *this, an exception is thrown.
    };

    class VariantIntegerData : public VariantDataBase
    {
            int mValue;

        public:

            VariantIntegerData (const VariantDataBase *data = nullptr);
            ///< Calling the constructor with an incompatible data type will result in a silent
            /// default initialisation.

            VariantDataBase *clone() const override;

            int getInteger (bool default_ = false) const override;
            ///< Will throw an exception, if value can not be represented as an integer (implicit
            /// casting of float values is permitted).
            ///
            /// \param default_ Return a default value instead of throwing an exception.

            float getFloat (bool default_ = false) const override;
            ///< Will throw an exception, if value can not be represented as a float value.
            ///
            /// \param default_ Return a default value instead of throwing an exception.

            void setInteger (int value) override;
            ///< Will throw an exception, if type is not compatible with integer.

            void setFloat (float value) override;
            ///< Will throw an exception, if type is not compatible with float.

            void read (ESMReader& esm, Variant::Format format, VarType type) override;
            ///< If \a type is not supported by \a format, an exception is thrown via ESMReader::fail

            void write (ESMWriter& esm, Variant::Format format, VarType type) const override;
            ///< If \a type is not supported by \a format, an exception is thrown.

            bool isEqual (const VariantDataBase& value) const override;
            ///< If the (C++) type of \a value does not match the type of *this, an exception is thrown.
    };

    class VariantFloatData : public VariantDataBase
    {
            float mValue;

        public:

            VariantFloatData (const VariantDataBase *data = nullptr);
            ///< Calling the constructor with an incompatible data type will result in a silent
            /// default initialisation.

            VariantDataBase *clone() const override;

            int getInteger (bool default_ = false) const override;
            ///< Will throw an exception, if value can not be represented as an integer (implicit
            /// casting of float values is permitted).
            ///
            /// \param default_ Return a default value instead of throwing an exception.

            float getFloat (bool default_ = false) const override;
            ///< Will throw an exception, if value can not be represented as a float value.
            ///
            /// \param default_ Return a default value instead of throwing an exception.

            void setInteger (int value) override;
            ///< Will throw an exception, if type is not compatible with integer.

            void setFloat (float value) override;
            ///< Will throw an exception, if type is not compatible with float.

            void read (ESMReader& esm, Variant::Format format, VarType type) override;
            ///< If \a type is not supported by \a format, an exception is thrown via ESMReader::fail

            void write (ESMWriter& esm, Variant::Format format, VarType type) const override;
            ///< If \a type is not supported by \a format, an exception is thrown.

            bool isEqual (const VariantDataBase& value) const override;
            ///< If the (C++) type of \a value does not match the type of *this, an exception is thrown.
    };
}

#endif
