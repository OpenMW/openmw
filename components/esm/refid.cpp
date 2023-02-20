#include "refid.hpp"

#include <ostream>
#include <sstream>
#include <stdexcept>

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
    }

    const RefId RefId::sEmpty = {};

    std::string EmptyRefId::toString() const
    {
        return std::string();
    }

    std::string EmptyRefId::toDebugString() const
    {
        return "Empty{}";
    }

    std::ostream& operator<<(std::ostream& stream, EmptyRefId value)
    {
        return stream << value.toDebugString();
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

    std::ostream& operator<<(std::ostream& stream, RefId value)
    {
        return std::visit([&](auto v) -> std::ostream& { return stream << v; }, value.mValue);
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
        return std::visit([](auto v) { return v.toDebugString(); }, mValue);
    }

    bool RefId::startsWith(std::string_view prefix) const
    {
        return std::visit(StartsWith{ prefix }, mValue);
    }

    bool RefId::contains(std::string_view subString) const
    {
        return std::visit(Contains{ subString }, mValue);
    }
}
