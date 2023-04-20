#ifndef OPENMW_COMPONENTS_ESM_FORMIDREFID_HPP
#define OPENMW_COMPONENTS_ESM_FORMIDREFID_HPP

#include <functional>
#include <iosfwd>
#include <stdexcept>

#include <components/esm/formid.hpp>

namespace ESM
{
    class FormIdRefId
    {
    public:
        constexpr FormIdRefId() = default;

        constexpr explicit FormIdRefId(ESM::FormId value)
            : mValue(value)
        {
            if ((mValue.mIndex & 0xff000000) != 0)
                throw std::invalid_argument("Invalid FormIdRefId index value: " + std::to_string(mValue.mIndex));
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
