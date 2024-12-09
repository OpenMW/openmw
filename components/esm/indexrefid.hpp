#ifndef OPENMW_COMPONENTS_ESM_INDEXREFID_HPP
#define OPENMW_COMPONENTS_ESM_INDEXREFID_HPP

#include <functional>
#include <iosfwd>

#include "defs.hpp"

namespace ESM
{
    class IndexRefId
    {
    public:
        constexpr explicit IndexRefId(RecNameInts recordType, std::uint32_t value) noexcept
            : mRecordType(recordType)
            , mValue(value)
        {
        }

        constexpr RecNameInts getRecordType() const { return mRecordType; }

        constexpr std::uint32_t getValue() const { return mValue; }

        std::string toString() const;

        std::string toDebugString() const;

        friend inline constexpr std::tuple<const ESM::RecNameInts&, const uint32_t&> tie(
            const IndexRefId& value) noexcept
        {
            return std::tie(value.mRecordType, value.mValue);
        }

        constexpr bool operator==(IndexRefId rhs) const noexcept { return tie(*this) == tie(rhs); }

        constexpr bool operator<(IndexRefId rhs) const noexcept { return tie(*this) < tie(rhs); }

        friend std::ostream& operator<<(std::ostream& stream, IndexRefId value);

        friend struct std::hash<IndexRefId>;

    private:
        RecNameInts mRecordType;
        std::uint32_t mValue;
    };

    static_assert(sizeof(IndexRefId) <= sizeof(std::uint64_t));
}

namespace std
{
    template <>
    struct hash<ESM::IndexRefId>
    {
        std::size_t operator()(ESM::IndexRefId value) const noexcept
        {
            return std::hash<std::uint64_t>{}((static_cast<std::uint64_t>(value.mRecordType) << 32) | value.mValue);
        }
    };
}

#endif
