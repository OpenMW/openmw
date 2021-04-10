#ifndef OPENMW_ESM_VARIANTIMP_H
#define OPENMW_ESM_VARIANTIMP_H

#include <string>
#include <functional>

#include "variant.hpp"

namespace ESM
{
    void readESMVariantValue(ESMReader& reader, Variant::Format format, VarType type, std::string& value);

    void readESMVariantValue(ESMReader& reader, Variant::Format format, VarType type, float& value);

    void readESMVariantValue(ESMReader& reader, Variant::Format format, VarType type, int& value);

    void writeESMVariantValue(ESMWriter& writer, Variant::Format format, VarType type, const std::string& value);

    void writeESMVariantValue(ESMWriter& writer, Variant::Format format, VarType type, float value);

    void writeESMVariantValue(ESMWriter& writer, Variant::Format format, VarType type, int value);

    struct ReadESMVariantValue
    {
        std::reference_wrapper<ESMReader> mReader;
        Variant::Format mFormat;
        VarType mType;

        ReadESMVariantValue(ESMReader& reader, Variant::Format format, VarType type)
            : mReader(reader), mFormat(format), mType(type) {}

        void operator()(std::monostate) const {}

        template <typename T>
        void operator()(T& value) const
        {
            readESMVariantValue(mReader.get(), mFormat, mType, value);
        }
    };

    struct WriteESMVariantValue
    {
        std::reference_wrapper<ESMWriter> mWriter;
        Variant::Format mFormat;
        VarType mType;

        WriteESMVariantValue(ESMWriter& writer, Variant::Format format, VarType type)
            : mWriter(writer), mFormat(format), mType(type) {}

        void operator()(std::monostate) const {}

        template <typename T>
        void operator()(const T& value) const
        {
            writeESMVariantValue(mWriter.get(), mFormat, mType, value);
        }
    };
}

#endif
