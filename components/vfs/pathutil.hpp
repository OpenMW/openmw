#ifndef OPENMW_COMPONENTS_VFS_PATHUTIL_H
#define OPENMW_COMPONENTS_VFS_PATHUTIL_H

#include <components/misc/strings/lower.hpp>

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace VFS::Path
{
    inline constexpr char normalize(char c)
    {
        return c == '\\' ? '/' : Misc::StringUtils::toLower(c);
    }

    inline constexpr bool isNormalized(std::string_view name)
    {
        return std::all_of(name.begin(), name.end(), [](char v) { return v == normalize(v); });
    }

    inline void normalizeFilenameInPlace(std::string& name)
    {
        std::transform(name.begin(), name.end(), name.begin(), normalize);
    }

    /// Normalize the given filename, making slashes/backslashes consistent, and lower-casing.
    [[nodiscard]] inline std::string normalizeFilename(std::string_view name)
    {
        std::string out(name);
        normalizeFilenameInPlace(out);
        return out;
    }

    struct PathCharLess
    {
        bool operator()(char x, char y) const { return normalize(x) < normalize(y); }
    };

    inline bool pathLess(std::string_view x, std::string_view y)
    {
        return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end(), PathCharLess());
    }

    inline bool pathEqual(std::string_view x, std::string_view y)
    {
        if (std::size(x) != std::size(y))
            return false;
        return std::equal(
            std::begin(x), std::end(x), std::begin(y), [](char l, char r) { return normalize(l) == normalize(r); });
    }

    struct PathLess
    {
        using is_transparent = void;

        bool operator()(std::string_view left, std::string_view right) const { return pathLess(left, right); }
    };

    class Normalized;

    class NormalizedView
    {
    public:
        constexpr NormalizedView() noexcept = default;

        constexpr NormalizedView(const char* value)
            : mValue(value)
        {
            if (!isNormalized(mValue))
                throw std::invalid_argument("NormalizedView value is not normalized: \"" + std::string(mValue) + "\"");
        }

        NormalizedView(const Normalized& value) noexcept;

        constexpr std::string_view value() const noexcept { return mValue; }

        friend constexpr bool operator==(const NormalizedView& lhs, const NormalizedView& rhs) = default;

        friend constexpr bool operator==(const NormalizedView& lhs, const auto& rhs) { return lhs.mValue == rhs; }

#if defined(_MSC_VER) && _MSC_VER <= 1935
        friend constexpr bool operator==(const auto& lhs, const NormalizedView& rhs)
        {
            return lhs == rhs.mValue;
        }
#endif

        friend constexpr bool operator<(const NormalizedView& lhs, const NormalizedView& rhs)
        {
            return lhs.mValue < rhs.mValue;
        }

        friend constexpr bool operator<(const NormalizedView& lhs, const auto& rhs)
        {
            return lhs.mValue < rhs;
        }

        friend constexpr bool operator<(const auto& lhs, const NormalizedView& rhs)
        {
            return lhs < rhs.mValue;
        }

        friend std::ostream& operator<<(std::ostream& stream, const NormalizedView& value)
        {
            return stream << value.mValue;
        }

    private:
        std::string_view mValue;
    };

    class Normalized
    {
    public:
        Normalized() = default;

        Normalized(std::string_view value)
            : mValue(normalizeFilename(value))
        {
        }

        Normalized(const char* value)
            : Normalized(std::string_view(value))
        {
        }

        Normalized(const std::string& value)
            : Normalized(std::string_view(value))
        {
        }

        explicit Normalized(std::string&& value)
            : mValue(std::move(value))
        {
            normalizeFilenameInPlace(mValue);
        }

        explicit Normalized(NormalizedView value)
            : mValue(value.value())
        {
        }

        const std::string& value() const& { return mValue; }

        std::string value() && { return std::move(mValue); }

        std::string_view view() const { return mValue; }

        operator std::string_view() const { return mValue; }

        operator const std::string&() const { return mValue; }

        friend bool operator==(const Normalized& lhs, const Normalized& rhs) = default;

        friend bool operator==(const Normalized& lhs, const auto& rhs) { return lhs.mValue == rhs; }

#if defined(_MSC_VER) && _MSC_VER <= 1935
        friend bool operator==(const auto& lhs, const Normalized& rhs)
        {
            return lhs == rhs.mValue;
        }
#endif

        friend bool operator==(const Normalized& lhs, const NormalizedView& rhs)
        {
            return lhs.mValue == rhs.value();
        }

        friend bool operator<(const Normalized& lhs, const Normalized& rhs)
        {
            return lhs.mValue < rhs.mValue;
        }

        friend bool operator<(const Normalized& lhs, const auto& rhs)
        {
            return lhs.mValue < rhs;
        }

        friend bool operator<(const auto& lhs, const Normalized& rhs)
        {
            return lhs < rhs.mValue;
        }

        friend bool operator<(const Normalized& lhs, const NormalizedView& rhs)
        {
            return lhs.mValue < rhs.value();
        }

        friend bool operator<(const NormalizedView& lhs, const Normalized& rhs)
        {
            return lhs.value() < rhs.mValue;
        }

        friend std::ostream& operator<<(std::ostream& stream, const Normalized& value)
        {
            return stream << value.mValue;
        }

    private:
        std::string mValue;
    };

    inline NormalizedView::NormalizedView(const Normalized& value) noexcept
        : mValue(value.view())
    {
    }
}

#endif
