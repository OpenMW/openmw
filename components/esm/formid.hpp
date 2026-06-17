#ifndef COMPONENT_ESM_FORMID_H
#define COMPONENT_ESM_FORMID_H

#include <cstdint>
#include <cstring>
#include <ostream>
#include <string>
#include <tuple>

namespace ESM
{
    using FormId32 = uint32_t;
    struct FormId
    {
        uint32_t mIndex = 0;
        int32_t mContentFile = -1;

        constexpr bool hasContentFile() const { return mContentFile >= 0; }
        constexpr bool isSet() const { return mIndex != 0 || mContentFile != -1; }

        // Zero is used in ESM4 as a null reference
        constexpr bool isZeroOrUnset() const { return mIndex == 0 && (mContentFile == 0 || mContentFile == -1); }

        std::string toString(std::string_view prefix = "") const;
        FormId32 toUint32() const;
        static constexpr FormId fromUint32(FormId32 v) { return { v & 0xffffff, static_cast<int32_t>(v >> 24) }; }
    };

    inline constexpr bool operator==(const FormId& left, const FormId& right)
    {
        return left.mIndex == right.mIndex && left.mContentFile == right.mContentFile;
    }

    inline constexpr bool operator<(const FormId& left, const FormId& right)
    {
        return std::tie(left.mIndex, left.mContentFile) < std::tie(right.mIndex, right.mContentFile);
    }

    inline std::ostream& operator<<(std::ostream& stream, const FormId& formId)
    {
        return stream << formId.toString();
    }
}

namespace std
{

    // Needed to use ESM::FormId as a key in std::unordered_map
    template <>
    struct hash<ESM::FormId>
    {
        size_t operator()(const ESM::FormId& formId) const
        {
            static_assert(sizeof(ESM::FormId) == sizeof(uint64_t));
            uint64_t s;
            memcpy(&s, &formId, sizeof(ESM::FormId));
            return hash<uint64_t>()(s);
        }
    };

}

#endif // COMPONENT_ESM_FORMID_H
