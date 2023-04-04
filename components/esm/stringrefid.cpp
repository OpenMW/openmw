#include "stringrefid.hpp"

#include <iomanip>
#include <mutex>
#include <ostream>
#include <sstream>
#include <unordered_set>

#include "components/misc/guarded.hpp"
#include "components/misc/strings/algorithm.hpp"

namespace ESM
{
    namespace
    {
        using StringsSet = std::unordered_set<std::string, Misc::StringUtils::CiHash, Misc::StringUtils::CiEqual>;

        const std::string emptyString;

        Misc::NotNullPtr<const std::string> getOrInsertString(std::string_view id)
        {
            static Misc::ScopeGuarded<StringsSet> refIds;
            const auto locked = refIds.lock();
            const std::string idString = std::string(id);
            auto it = locked->find(idString);
            if (it == locked->end())
                it = locked->emplace(id).first;
            return &*it;
        }
    }

    StringRefId::StringRefId()
        : mValue(&emptyString)
    {
    }

    StringRefId::StringRefId(std::string_view value)
        : mValue(getOrInsertString(value))
    {
    }

    bool StringRefId::operator==(std::string_view rhs) const noexcept
    {
        return Misc::StringUtils::ciEqual(*mValue, rhs);
    }

    bool StringRefId::operator<(StringRefId rhs) const noexcept
    {
        return Misc::StringUtils::ciLess(*mValue, *rhs.mValue);
    }

    bool operator<(StringRefId lhs, std::string_view rhs) noexcept
    {
        return Misc::StringUtils::ciLess(*lhs.mValue, rhs);
    }

    bool operator<(std::string_view lhs, StringRefId rhs) noexcept
    {
        return Misc::StringUtils::ciLess(lhs, *rhs.mValue);
    }

    std::ostream& operator<<(std::ostream& stream, StringRefId value)
    {
        stream << '"';
        for (char c : *value.mValue)
            if (std::isprint(c) && c != '\t' && c != '\n' && c != '\r')
                stream << c;
            else
                stream << "\\x" << std::hex << std::uppercase << static_cast<unsigned>(static_cast<unsigned char>(c));
        return stream << '"';
    }

    std::string StringRefId::toDebugString() const
    {
        std::ostringstream stream;
        stream << *this;
        return stream.str();
    }

    bool StringRefId::startsWith(std::string_view prefix) const
    {
        return Misc::StringUtils::ciStartsWith(*mValue, prefix);
    }

    bool StringRefId::endsWith(std::string_view suffix) const
    {
        return Misc::StringUtils::ciEndsWith(*mValue, suffix);
    }

    bool StringRefId::contains(std::string_view subString) const
    {
        return Misc::StringUtils::ciFind(*mValue, subString) != std::string_view::npos;
    }
}
