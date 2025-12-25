#ifndef OPENMW_COMPONENTS_ESM_STRINGREFID_HPP
#define OPENMW_COMPONENTS_ESM_STRINGREFID_HPP

#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

#include <components/misc/notnullptr.hpp>

namespace ESM
{
    class StringRefId
    {
    public:
        StringRefId();

        // Constructs StringRefId from string using pointer to a static set of strings.
        explicit StringRefId(std::string_view value);

        const std::string& getValue() const { return *mValue; }

        std::string toString() const { return *mValue; }

        std::string toDebugString() const;

        bool startsWith(std::string_view prefix) const;

        bool endsWith(std::string_view suffix) const;

        bool contains(std::string_view subString) const;

        bool operator==(StringRefId rhs) const noexcept { return mValue == rhs.mValue; }

        bool operator==(std::string_view rhs) const noexcept;

        // Compares values to provide stable order
        bool operator<(StringRefId rhs) const noexcept;

        friend bool operator<(StringRefId lhs, std::string_view rhs) noexcept;

        friend bool operator<(std::string_view lhs, StringRefId rhs) noexcept;

        friend std::ostream& operator<<(std::ostream& stream, StringRefId value);

        friend struct std::hash<StringRefId>;

        // Similar to the constructor but only returns preexisting ids
        static std::optional<StringRefId> deserializeExisting(std::string_view value);

    private:
        Misc::NotNullPtr<const std::string> mValue;
    };
}

namespace std
{
    template <>
    struct hash<ESM::StringRefId>
    {
        std::size_t operator()(ESM::StringRefId value) const noexcept
        {
            return std::hash<const std::string*>{}(value.mValue);
        }
    };
}

#endif
