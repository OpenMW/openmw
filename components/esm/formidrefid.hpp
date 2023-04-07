#ifndef OPENMW_COMPONENTS_ESM_FORMIDREFID_HPP
#define OPENMW_COMPONENTS_ESM_FORMIDREFID_HPP

#include <functional>
#include <iosfwd>

#include <components/esm/formid.hpp>

namespace ESM
{
    class FormIdRefId
    {
    public:
        constexpr FormIdRefId() = default;

        constexpr explicit FormIdRefId(ESM::FormId value) noexcept
            : mValue(value)
        {
        }

        ESM::FormId getValue() const { return mValue; }

        std::string toString() const;

        std::string toDebugString() const;

        constexpr bool operator==(FormIdRefId rhs) const noexcept { return mValue == rhs.mValue; }

        constexpr bool operator<(FormIdRefId rhs) const noexcept { return mValue < rhs.mValue; }

        friend std::ostream& operator<<(std::ostream& stream, FormIdRefId value);

        friend struct std::hash<FormIdRefId>;

    private:
        ESM::FormId mValue;
    };
}

namespace std
{
    template <>
    struct hash<ESM::FormIdRefId>
    {
        std::size_t operator()(ESM::FormIdRefId value) const noexcept { return std::hash<ESM::FormId>{}(value.mValue); }
    };
}

#endif
