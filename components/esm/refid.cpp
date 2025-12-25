#include "refid.hpp"

#include "serializerefid.hpp"

#include "components/misc/strings/lower.hpp"

#include <sstream>
#include <stdexcept>
#include <variant>

namespace ESM
{
    namespace
    {
        const std::string emptyString;

        struct GetRefString
        {
            const std::string& operator()(EmptyRefId /*v*/) const { return emptyString; }

            const std::string& operator()(StringRefId v) const { return v.getValue(); }

            template <class T>
            const std::string& operator()(const T& v) const
            {
                std::ostringstream stream;
                stream << "RefId is not a string: " << v;
                throw std::runtime_error(stream.str());
            }
        };

        struct IsEqualToString
        {
            const std::string_view mRhs;

            bool operator()(StringRefId v) const noexcept { return v == mRhs; }

            template <class T>
            bool operator()(const T& /*v*/) const noexcept
            {
                return false;
            }
        };

        struct IsLessThanString
        {
            const std::string_view mRhs;

            bool operator()(StringRefId v) const noexcept { return v < mRhs; }

            template <class T>
            bool operator()(const T& /*v*/) const noexcept
            {
                return false;
            }
        };

        struct IsGreaterThanString
        {
            const std::string_view mLhs;

            bool operator()(StringRefId v) const noexcept { return mLhs < v; }

            template <class T>
            bool operator()(const T& /*v*/) const noexcept
            {
                return true;
            }
        };

        struct StartsWith
        {
            const std::string_view mPrefix;

            bool operator()(StringRefId v) const { return v.startsWith(mPrefix); }

            template <class T>
            bool operator()(const T& /*v*/) const
            {
                return false;
            }
        };

        struct EndsWith
        {
            const std::string_view mSuffix;

            bool operator()(StringRefId v) const { return v.endsWith(mSuffix); }

            template <class T>
            bool operator()(const T& /*v*/) const
            {
                return false;
            }
        };

        struct Contains
        {
            const std::string_view mSubString;

            bool operator()(StringRefId v) const { return v.contains(mSubString); }

            template <class T>
            bool operator()(const T& /*v*/) const
            {
                return false;
            }
        };

        struct SerializeText
        {
            std::string operator()(ESM::EmptyRefId /*v*/) const { return std::string(); }

            std::string operator()(ESM::StringRefId v) const { return Misc::StringUtils::lowerCase(v.getValue()); }

            std::string operator()(ESM::FormId v) const { return v.toString(formIdRefIdPrefix); }

            template <class T>
            std::string operator()(const T& v) const
            {
                return v.toDebugString();
            }
        };
    }

    bool RefId::operator==(std::string_view rhs) const
    {
        return std::visit(IsEqualToString{ rhs }, mValue);
    }

    bool operator<(RefId lhs, std::string_view rhs)
    {
        return std::visit(IsLessThanString{ rhs }, lhs.mValue);
    }

    bool operator<(std::string_view lhs, RefId rhs)
    {
        return std::visit(IsGreaterThanString{ lhs }, rhs.mValue);
    }

    RefId RefId::stringRefId(std::string_view value)
    {
        if (value.empty())
            return RefId();
        return RefId(StringRefId(value));
    }

    const std::string& RefId::getRefIdString() const
    {
        return std::visit(GetRefString{}, mValue);
    }

    std::string RefId::toString() const
    {
        return std::visit([](auto v) { return v.toString(); }, mValue);
    }

    std::string RefId::toDebugString() const
    {
        return std::visit(
            [](auto v) {
                if constexpr (std::is_same_v<decltype(v), FormId>)
                    return v.toString(formIdRefIdPrefix);
                else
                    return v.toDebugString();
            },
            mValue);
    }

    bool RefId::startsWith(std::string_view prefix) const
    {
        return std::visit(StartsWith{ prefix }, mValue);
    }

    bool RefId::endsWith(std::string_view suffix) const
    {
        return std::visit(EndsWith{ suffix }, mValue);
    }

    bool RefId::contains(std::string_view subString) const
    {
        return std::visit(Contains{ subString }, mValue);
    }

    std::string RefId::serialize() const
    {
        std::string result(sizeof(mValue), '\0');
        std::memcpy(result.data(), &mValue, sizeof(mValue));
        return result;
    }

    ESM::RefId RefId::deserialize(std::string_view value)
    {
        if (value.size() != sizeof(ESM::RefId::mValue))
            throw std::runtime_error("Invalid value size to deserialize: " + std::to_string(value.size()));
        ESM::RefId result;
        std::memcpy(&result.mValue, value.data(), sizeof(result.mValue));
        return result;
    }

    std::string RefId::serializeText() const
    {
        return std::visit(SerializeText{}, mValue);
    }

    ESM::RefId RefId::deserializeText(std::string_view value)
    {
        if (value.empty())
            return ESM::RefId();

        if (value.starts_with(formIdRefIdPrefix))
        {
            uint64_t v = deserializeHexIntegral<uint64_t>(formIdRefIdPrefix.size(), value);
            uint32_t index = static_cast<uint32_t>(v) & 0xffffff;
            int contentFile = static_cast<int>(v >> 24);
            return ESM::FormId{ index, contentFile };
        }

        if (value.starts_with(generatedRefIdPrefix))
            return ESM::RefId::generated(deserializeHexIntegral<std::uint64_t>(generatedRefIdPrefix.size(), value));

        if (value.starts_with(indexRefIdPrefix))
        {
            ESM::RecNameInts recordType{};
            std::memcpy(&recordType, value.data() + indexRefIdPrefix.size(), sizeof(recordType));
            return ESM::RefId::index(recordType,
                deserializeHexIntegral<std::uint32_t>(indexRefIdPrefix.size() + sizeof(recordType) + 1, value));
        }

        if (value.starts_with(esm3ExteriorCellRefIdPrefix))
        {
            if (value.size() < esm3ExteriorCellRefIdPrefix.size() + 3)
                throw std::runtime_error("Invalid ESM3ExteriorCellRefId format: not enough size");
            const std::size_t separator = value.find(':', esm3ExteriorCellRefIdPrefix.size() + 1);
            if (separator == std::string_view::npos)
                throw std::runtime_error("Invalid ESM3ExteriorCellRefId format: coordinates separator is not found");
            const std::int32_t x
                = deserializeDecIntegral<std::int32_t>(esm3ExteriorCellRefIdPrefix.size(), separator, value);
            const std::int32_t y = deserializeDecIntegral<std::int32_t>(separator + 1, value.size(), value);
            return ESM::ESM3ExteriorCellRefId(x, y);
        }

        if (auto id = ESM::StringRefId::deserializeExisting(value))
            return *id;
        return {};
    }
}
