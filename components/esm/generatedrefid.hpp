#ifndef OPENMW_COMPONENTS_ESM_GENERATEDREFID_HPP
#define OPENMW_COMPONENTS_ESM_GENERATEDREFID_HPP

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <string>

namespace ESM
{
    class GeneratedRefId
    {
    public:
        constexpr explicit GeneratedRefId(std::uint64_t value) noexcept
            : mValue(value)
        {
        }

        constexpr std::uint64_t getValue() const { return mValue; }

        std::string toString() const;

        std::string toDebugString() const;

        constexpr bool operator==(GeneratedRefId rhs) const noexcept { return mValue == rhs.mValue; }

        constexpr bool operator<(GeneratedRefId rhs) const noexcept { return mValue < rhs.mValue; }

        friend std::ostream& operator<<(std::ostream& stream, GeneratedRefId value);

        friend struct std::hash<GeneratedRefId>;

    private:
        std::uint64_t mValue;
    };
}

namespace std
{
    template <>
    struct hash<ESM::GeneratedRefId>
    {
        std::size_t operator()(ESM::GeneratedRefId value) const noexcept
        {
            return std::hash<std::uint64_t>{}(value.mValue);
        }
    };
}

#endif
