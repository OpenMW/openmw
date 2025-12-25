#include "stringrefid.hpp"
#include "serializerefid.hpp"

#include <charconv>
#include <ostream>
#include <system_error>
#include <unordered_set>

#include "components/misc/guarded.hpp"
#include "components/misc/strings/algorithm.hpp"
#include "components/misc/utf8stream.hpp"

namespace ESM
{
    namespace
    {
        using StringsSet = std::unordered_set<std::string, Misc::StringUtils::CiHash, Misc::StringUtils::CiEqual>;

        const std::string emptyString;

        Misc::ScopeGuarded<StringsSet>& getRefIds()
        {
            static Misc::ScopeGuarded<StringsSet> refIds;
            return refIds;
        }

        Misc::NotNullPtr<const std::string> getOrInsertString(std::string_view id)
        {
            const auto locked = getRefIds().lock();
            auto it = locked->find(id);
            if (it == locked->end())
                it = locked->emplace(id).first;
            return &*it;
        }

        void addHex(unsigned char value, std::string& result)
        {
            const std::size_t size = 2 + getHexIntegralSize(value);
            const std::size_t shift = result.size();
            result.resize(shift + size);
            result[shift] = '\\';
            result[shift + 1] = 'x';
            const auto [end, ec] = std::to_chars(result.data() + shift + 2, result.data() + result.size(), value, 16);
            if (ec != std::errc())
                throw std::system_error(std::make_error_code(ec));
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
        return stream << value.toDebugString();
    }

    std::string StringRefId::toDebugString() const
    {
        std::string result;
        result.reserve(2 + mValue->size());
        result.push_back('"');
        const unsigned char* ptr = reinterpret_cast<const unsigned char*>(mValue->data());
        const unsigned char* const end = reinterpret_cast<const unsigned char*>(mValue->data() + mValue->size());
        while (ptr != end)
        {
            if (Utf8Stream::isAscii(*ptr))
            {
                if (std::isprint(*ptr) && *ptr != '\t' && *ptr != '\n' && *ptr != '\r')
                    result.push_back(*ptr);
                else
                    addHex(*ptr, result);
                ++ptr;
                continue;
            }
            const auto [octets, first] = Utf8Stream::getOctetCount(*ptr);
            const auto [chr, next] = Utf8Stream::decode(ptr + 1, end, first, octets);
            if (chr == Utf8Stream::sBadChar())
            {
                while (ptr != std::min(end, ptr + octets))
                {
                    addHex(*ptr, result);
                    ++ptr;
                }
                continue;
            }
            result.append(ptr, next);
            ptr = next;
        }
        result.push_back('"');
        return result;
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

    std::optional<StringRefId> StringRefId::deserializeExisting(std::string_view value)
    {
        const auto locked = getRefIds().lock();
        auto it = locked->find(value);
        if (it == locked->end())
            return {};
        StringRefId id;
        id.mValue = &*it;
        return id;
    }
}
