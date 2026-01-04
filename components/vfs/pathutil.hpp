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

    [[nodiscard]] inline constexpr char normalize(char c)
    {
        return c == '\\' ? separator : Misc::StringUtils::toLower(c);
    }

    [[nodiscard]] inline constexpr bool isNormalized(std::string_view name)
    {
        if (name.empty())
            return true;

        if (name.front() != normalize(name.front()))
            return false;

        if (name.front() == separator)
            return false;

        for (std::size_t i = 1, n = name.size(); i < n; ++i)
        {
            if (name[i] != normalize(name[i]))
                return false;

            if (name[i] == separator && name[i - 1] == name[i])
                return false;
        }

        return true;
    }

    [[nodiscard]] inline auto removeDuplicatedSeparators(auto begin, auto end)
    {
        return std::unique(begin, end, [](char a, char b) { return a == separator && b == separator; });
    }

    [[nodiscard]] inline auto removeLeadingSeparator(auto begin, auto end)
    {
        if (begin != end && *begin == separator)
            return begin + 1;
        return begin;
    }

    [[nodiscard]] inline auto normalizeFilenameInPlace(auto begin, auto end)
    {
        std::transform(begin, end, begin, normalize);
        end = removeDuplicatedSeparators(begin, end);
        begin = removeLeadingSeparator(begin, end);
        return std::pair(begin, end);
    }

    inline void normalizeFilenameInPlace(std::string& name)
    {
        const auto [begin, end] = normalizeFilenameInPlace(name.begin(), name.end());
        name.erase(end, name.end());
        name.erase(name.begin(), begin);
    }

    /// Normalize the given filename, making slashes/backslashes consistent, and lower-casing.
    [[nodiscard]] inline std::string normalizeFilename(std::string_view name)
    {
        std::string out(name);
        normalizeFilenameInPlace(out);
        return out;
    }

    inline constexpr auto findSeparatorOrExtensionSeparator(auto begin, auto end)
    {
        return std::find_if(begin, end, [](char v) { return v == extensionSeparator || v == separator; });
    }

    inline constexpr bool isExtension(std::string_view value)
    {
        return isNormalized(value) && findSeparatorOrExtensionSeparator(value.begin(), value.end()) == value.end();
    }

    class NormalizedView;

    class ExtensionView
    {
    public:
        constexpr ExtensionView() noexcept = default;

        constexpr explicit ExtensionView(const char* value)
            : mValue(value)
        {
            if (!isExtension(mValue))
                throw std::invalid_argument(
                    "ExtensionView value is invalid extension: \"" + std::string(mValue) + "\"");
        }

        constexpr std::string_view value() const noexcept { return mValue; }

        constexpr bool empty() const noexcept { return mValue.empty(); }

        friend constexpr bool operator==(const ExtensionView& lhs, const ExtensionView& rhs) = default;

        friend constexpr bool operator==(const ExtensionView& lhs, const auto& rhs) { return lhs.mValue == rhs; }

#if defined(_MSC_VER) && _MSC_VER <= 1935
        friend constexpr bool operator==(const auto& lhs, const ExtensionView& rhs)
        {
            return lhs == rhs.mValue;
        }
#endif

        friend constexpr bool operator<(const ExtensionView& lhs, const ExtensionView& rhs)
        {
            return lhs.mValue < rhs.mValue;
        }

        friend constexpr bool operator<(const ExtensionView& lhs, const auto& rhs)
        {
            return lhs.mValue < rhs;
        }

        friend constexpr bool operator<(const auto& lhs, const ExtensionView& rhs)
        {
            return lhs < rhs.mValue;
        }

        friend std::ostream& operator<<(std::ostream& stream, const ExtensionView& value)
        {
            return stream << value.mValue;
        }

    private:
        std::string_view mValue;

        friend class NormalizedView;
    };

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

        NormalizedView parent() const
        {
            NormalizedView p;
            const std::size_t pos = mValue.find_last_of(separator);
            if (pos != std::string_view::npos)
                p.mValue = mValue.substr(0, pos);
            return p;
        }

        NormalizedView filename() const
        {
            NormalizedView result(*this);
            if (const std::size_t position = mValue.find_last_of(separator); position != std::string_view::npos)
                result.mValue.remove_prefix(position + 1);
            return result;
        }

        std::string_view stem() const
        {
            std::string_view stem = filename().value();
            if (const std::size_t pos = stem.find_last_of(extensionSeparator); pos != std::string_view::npos)
                stem = stem.substr(0, pos);
            return stem;
        }

        constexpr ExtensionView extension() const
        {
            ExtensionView result;
            if (const std::size_t position = mValue.find_last_of(extensionSeparator);
                position != std::string_view::npos)
                result.mValue = mValue.substr(position + 1);
            return result;
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

        bool changeExtension(ExtensionView extension)
        {
            const auto it = findSeparatorOrExtensionSeparator(mValue.rbegin(), mValue.rend());
            if (it == mValue.rend() || *it == separator)
                return false;
            const std::string::difference_type pos = mValue.rend() - it;
            mValue.replace(pos, mValue.size(), extension.value());
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
            const auto [begin, end] = normalizeFilenameInPlace(mValue.begin() + offset, mValue.end());
            std::copy(begin, end, mValue.begin() + offset);
            mValue.resize(offset + (end - begin));
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

        NormalizedView parent() const
        {
            return NormalizedView(*this).parent();
        }

        std::string_view stem() const
        {
            return NormalizedView(*this).stem();
        }

        NormalizedView filename() const
        {
            return NormalizedView(*this).filename();
        }

        ExtensionView extension() const
        {
            return NormalizedView(*this).extension();
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
