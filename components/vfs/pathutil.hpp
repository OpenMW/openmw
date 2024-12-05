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
    inline constexpr char separator = '/';
    inline constexpr char extensionSeparator = '.';

    inline constexpr char normalize(char c)
    {
        return c == '\\' ? separator : Misc::StringUtils::toLower(c);
    }

    inline constexpr bool isNormalized(std::string_view name)
    {
        return std::all_of(name.begin(), name.end(), [](char v) { return v == normalize(v); });
    }

    inline void normalizeFilenameInPlace(auto begin, auto end)
    {
        std::transform(begin, end, begin, normalize);
    }

    inline void normalizeFilenameInPlace(std::string& name)
    {
        normalizeFilenameInPlace(name.begin(), name.end());
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

    inline constexpr auto findSeparatorOrExtensionSeparator(auto begin, auto end)
    {
        return std::find_if(begin, end, [](char v) { return v == extensionSeparator || v == separator; });
    }

    class Normalized;

    class NormalizedView
    {
    public:
        constexpr NormalizedView() noexcept = default;

        constexpr explicit NormalizedView(const char* value)
            : mValue(value)
        {
            if (!isNormalized(mValue))
                throw std::invalid_argument("NormalizedView value is not normalized: \"" + std::string(mValue) + "\"");
        }

        NormalizedView(const Normalized& value) noexcept;

        explicit NormalizedView(const std::string&) = delete;

        explicit NormalizedView(std::string&&) = delete;

        constexpr std::string_view value() const noexcept { return mValue; }

        constexpr bool empty() const noexcept { return mValue.empty(); }

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

        explicit Normalized(std::string_view value)
            : mValue(normalizeFilename(value))
        {
        }

        explicit Normalized(const char* value)
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

        bool empty() const { return mValue.empty(); }

        operator std::string_view() const { return mValue; }

        operator const std::string&() const { return mValue; }

        bool changeExtension(std::string_view extension)
        {
            if (findSeparatorOrExtensionSeparator(extension.begin(), extension.end()) != extension.end())
                throw std::invalid_argument("Invalid extension: " + std::string(extension));
            const auto it = findSeparatorOrExtensionSeparator(mValue.rbegin(), mValue.rend());
            if (it == mValue.rend() || *it == separator)
                return false;
            const std::string::difference_type pos = mValue.rend() - it;
            mValue.replace(pos, mValue.size(), extension);
            normalizeFilenameInPlace(mValue.begin() + pos, mValue.end());
            return true;
        }

        void clear() { mValue.clear(); }

        Normalized& operator=(NormalizedView value)
        {
            mValue = value.value();
            return *this;
        }

        Normalized& operator/=(NormalizedView value)
        {
            mValue.reserve(mValue.size() + value.value().size() + 1);
            mValue += separator;
            mValue += value.value();
            return *this;
        }

        Normalized& operator/=(std::string_view value)
        {
            mValue.reserve(mValue.size() + value.size() + 1);
            mValue += separator;
            const std::size_t offset = mValue.size();
            mValue += value;
            normalizeFilenameInPlace(mValue.begin() + offset, mValue.end());
            return *this;
        }

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

    inline Normalized operator/(NormalizedView lhs, NormalizedView rhs)
    {
        Normalized result(lhs);
        result /= rhs;
        return result;
    }

    struct Hash
    {
        using is_transparent = void;

        [[nodiscard]] std::size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }

        [[nodiscard]] std::size_t operator()(const std::string& s) const { return std::hash<std::string>{}(s); }

        [[nodiscard]] std::size_t operator()(const Normalized& s) const { return std::hash<std::string>{}(s.value()); }

        [[nodiscard]] std::size_t operator()(NormalizedView s) const
        {
            return std::hash<std::string_view>{}(s.value());
        }
    };

    // A special function to be removed once conversion to VFS::Path::Normalized* is complete
    template <class T>
    Normalized toNormalized(T&& value)
    {
        return Normalized(std::forward<T>(value));
    }

    Normalized toNormalized(NormalizedView value) = delete;

    Normalized toNormalized(Normalized value) = delete;
}

#endif
