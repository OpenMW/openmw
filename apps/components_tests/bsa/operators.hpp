#ifndef COMPONETS_TESTS_BSA_OPERATORS_H
#define COMPONETS_TESTS_BSA_OPERATORS_H

#include <components/bsa/bsafile.hpp>

#include <ostream>
#include <tuple>

namespace Bsa
{
    inline auto makeTuple(const BSAFile::Hash& value)
    {
        return std::make_tuple(value.mLow, value.mHigh);
    }

    inline auto makeTuple(const BSAFile::FileStruct& value)
    {
        return std::make_tuple(
            value.mFileSize, value.mOffset, makeTuple(value.mHash), value.mNameOffset, value.mNameSize, value.name());
    }

    inline std::ostream& operator<<(std::ostream& stream, const BSAFile::Hash& value)
    {
        return stream << "Hash { .mLow = " << value.mLow << ", .mHigh = " << value.mHigh << "}";
    }

    inline std::ostream& operator<<(std::ostream& stream, const BSAFile::FileStruct& value)
    {
        return stream << "FileStruct { .mFileSize = " << value.mFileSize << ", .mOffset = " << value.mOffset
                      << ", .mHash = " << value.mHash << ", .mNameOffset = " << value.mNameOffset
                      << ", .mNameSize = " << value.mNameSize << ", .name() = " << value.name() << "}";
    }

    inline bool operator==(const BSAFile::FileStruct& lhs, const BSAFile::FileStruct& rhs)
    {
        return makeTuple(lhs) == makeTuple(rhs);
    }
}

#endif
