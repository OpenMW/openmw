#ifndef OPENMW_COMPONENTS_ESM_REFID_HPP
#define OPENMW_COMPONENTS_ESM_REFID_HPP

#include <functional>
#include <iosfwd>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#include <components/misc/notnullptr.hpp>

#include "esm3exteriorcellrefid.hpp"
#include "formid.hpp"
#include "generatedrefid.hpp"
#include "indexrefid.hpp"
#include "stringrefid.hpp"

namespace ESM
{
    struct EmptyRefId
    {
        constexpr bool operator==(EmptyRefId /*rhs*/) const { return true; }

        constexpr bool operator<(EmptyRefId /*rhs*/) const { return false; }

        std::string toString() const { return ""; }

        std::string toDebugString() const { return "Empty{}"; }
    };

    enum class RefIdType : std::uint8_t
    {
        Empty = 0,
        SizedString = 1,
        UnsizedString = 2,
        FormId = 3,
        Generated = 4,
        Index = 5,
        ESM3ExteriorCell = 6,
    };

    // RefId is used to represent an Id that identifies an ESM record. These Ids can then be used in
    // ESM::Stores to find the actual record. These Ids can be serialized/de-serialized, stored on disk and remain
    // valid. They are used by ESM files, by records to reference other ESM records.
    class RefId
    {
    public:
        using Value = std::variant<EmptyRefId, ESM3ExteriorCellRefId, StringRefId, FormId, GeneratedRefId, IndexRefId>;

        // Constructs RefId from a serialized string containing byte by byte copy of RefId::mValue.
        static ESM::RefId deserialize(std::string_view value);

        // Constructs RefId from a serialized text.
        static ESM::RefId deserializeText(std::string_view value);

        // Constructs RefId from a string using a pointer to a static set of strings.
        static RefId stringRefId(std::string_view value);

        // Constructs RefId from FormId storing the value in-place.
        static RefId formIdRefId(FormId value) { return RefId(value); }

        // Constructs RefId from uint64 storing the value in-place. Should be used for generated records where id is a
        // global counter.
        static RefId generated(std::uint64_t value) { return RefId(GeneratedRefId{ value }); }

        // Constructs RefId from record type and 32bit index storing the value in-place. Should be used for records
        // identified by index (i.e. ESM3 SKIL).
        static RefId index(RecNameInts recordType, std::uint32_t value) { return RefId(IndexRefId(recordType, value)); }

        static RefId esm3ExteriorCell(int32_t x, int32_t y) { return RefId(ESM3ExteriorCellRefId(x, y)); }

        constexpr RefId() noexcept
            : mValue(EmptyRefId{})
        {
        }

        constexpr RefId(EmptyRefId value) noexcept
            : mValue(value)
        {
        }

        RefId(StringRefId value) noexcept
            : mValue(value)
        {
        }

        constexpr RefId(FormId value)
        {
            if (value.isZeroOrUnset())
                mValue = EmptyRefId();
            else if (value.hasContentFile())
                mValue = value;
            else
                throw std::logic_error("RefId can't be a generated FormId");
        }

        constexpr RefId(GeneratedRefId value) noexcept
            : mValue(value)
        {
        }

        constexpr RefId(IndexRefId value) noexcept
            : mValue(value)
        {
        }

        constexpr RefId(ESM3ExteriorCellRefId value) noexcept
            : mValue(value)
        {
        }

        // Returns a reference to the value of StringRefId if it's the underlying value or throws an exception.
        const std::string& getRefIdString() const;

        // Returns a string with serialized underlying value.
        std::string toString() const;

        // Returns a string with serialized underlying value with information about underlying type.
        std::string toDebugString() const;

        constexpr bool empty() const noexcept { return std::holds_alternative<EmptyRefId>(mValue); }

        // Returns true if underlying value is StringRefId and its underlying std::string starts with given prefix.
        // Otherwise returns false.
        bool startsWith(std::string_view prefix) const;

        // Returns true if underlying value is StringRefId and its underlying std::string ends with given suffix.
        // Otherwise returns false.
        bool endsWith(std::string_view suffix) const;

        // Returns true if underlying value is StringRefId and its underlying std::string contains given subString.
        // Otherwise returns false.
        bool contains(std::string_view subString) const;

        // Copy mValue byte by byte into a string. Use result only during within the same process.
        std::string serialize() const;

        // Serialize into stable text format.
        std::string serializeText() const;

        template <class T>
        const T* getIf() const
        {
            return std::get_if<T>(&mValue);
        }

        template <class T>
        bool is() const
        {
            return std::holds_alternative<T>(mValue);
        }

        friend constexpr bool operator==(const RefId& l, const RefId& r) { return l.mValue == r.mValue; }

        bool operator==(std::string_view rhs) const;

        friend constexpr bool operator<(const RefId& l, const RefId& r) { return l.mValue < r.mValue; }

        friend bool operator<(RefId lhs, std::string_view rhs);

        friend bool operator<(std::string_view lhs, RefId rhs);

        friend std::ostream& operator<<(std::ostream& stream, RefId value) { return stream << value.toDebugString(); }

        template <class F, class... T>
        friend constexpr auto visit(F&& f, T&&... v)
            -> std::enable_if_t<(std::is_same_v<std::decay_t<T>, RefId> && ...),
                decltype(std::visit(std::forward<F>(f), std::forward<T>(v).mValue...))>
        {
            return std::visit(std::forward<F>(f), std::forward<T>(v).mValue...);
        }

        friend struct std::hash<ESM::RefId>;

    private:
        Value mValue{ EmptyRefId{} };
    };
}

namespace std
{
    template <>
    struct hash<ESM::EmptyRefId>
    {
        std::size_t operator()(ESM::EmptyRefId /*value*/) const noexcept { return 0; }
    };

    template <>
    struct hash<ESM::RefId>
    {
        std::size_t operator()(ESM::RefId value) const noexcept
        {
            return std::hash<decltype(value.mValue)>{}(value.mValue);
        }
    };
}

#endif
