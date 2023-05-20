#ifndef OPENMW_COMPONENTS_SETTINGS_SETTINGVALUE_H
#define OPENMW_COMPONENTS_SETTINGS_SETTINGVALUE_H

#include "sanitizer.hpp"
#include "settings.hpp"

#include "components/debug/debuglog.hpp"

#include <osg/io_utils>

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace Settings
{
    enum class SettingValueType
    {
        Bool,
        Int,
        UnsignedInt,
        Long,
        UnsignedLong,
        LongLong,
        UnsignedLongLong,
        Float,
        Double,
        String,
        Vec2f,
        Vec3f,
    };

    template <class T>
    constexpr SettingValueType getSettingValueType();

    template <>
    inline constexpr SettingValueType getSettingValueType<bool>()
    {
        return SettingValueType::Bool;
    }

    template <>
    inline constexpr SettingValueType getSettingValueType<int>()
    {
        return SettingValueType::Int;
    }

    template <>
    inline constexpr SettingValueType getSettingValueType<unsigned int>()
    {
        return SettingValueType::UnsignedInt;
    }

    template <>
    inline constexpr SettingValueType getSettingValueType<long>()
    {
        return SettingValueType::Long;
    }

    template <>
    inline constexpr SettingValueType getSettingValueType<unsigned long>()
    {
        return SettingValueType::UnsignedLong;
    }

    template <>
    inline constexpr SettingValueType getSettingValueType<long long>()
    {
        return SettingValueType::LongLong;
    }

    template <>
    inline constexpr SettingValueType getSettingValueType<unsigned long long>()
    {
        return SettingValueType::UnsignedLongLong;
    }

    template <>
    inline constexpr SettingValueType getSettingValueType<float>()
    {
        return SettingValueType::Float;
    }

    template <>
    inline constexpr SettingValueType getSettingValueType<double>()
    {
        return SettingValueType::Double;
    }

    template <>
    inline constexpr SettingValueType getSettingValueType<std::string>()
    {
        return SettingValueType::String;
    }

    template <>
    inline constexpr SettingValueType getSettingValueType<osg::Vec2f>()
    {
        return SettingValueType::Vec2f;
    }

    template <>
    inline constexpr SettingValueType getSettingValueType<osg::Vec3f>()
    {
        return SettingValueType::Vec3f;
    }

    inline constexpr std::string_view getSettingValueTypeName(SettingValueType type)
    {
        switch (type)
        {
            case SettingValueType::Bool:
                return "bool";
            case SettingValueType::Int:
                return "int";
            case SettingValueType::UnsignedInt:
                return "unsigned int";
            case SettingValueType::Long:
                return "long";
            case SettingValueType::UnsignedLong:
                return "unsigned long";
            case SettingValueType::LongLong:
                return "long long";
            case SettingValueType::UnsignedLongLong:
                return "unsigned long long";
            case SettingValueType::Float:
                return "float";
            case SettingValueType::Double:
                return "double";
            case SettingValueType::String:
                return "string";
            case SettingValueType::Vec2f:
                return "vec2f";
            case SettingValueType::Vec3f:
                return "vec3f";
        }
        return "unsupported";
    }

    template <class T>
    constexpr std::string_view getSettingValueTypeName()
    {
        return getSettingValueTypeName(getSettingValueType<T>());
    }

    inline std::string getSettingDescription(SettingValueType type, std::string_view category, std::string_view name)
    {
        return std::string(getSettingValueTypeName(type)) + " [" + std::string(category) + "] " + std::string(name)
            + " setting";
    }

    template <class T>
    std::string getSettingDescription(std::string_view category, std::string_view name)
    {
        return getSettingDescription(getSettingValueType<T>(), category, name);
    }

    class Index;

    class BaseSettingValue
    {
    public:
        const SettingValueType mType;
        const std::string_view mCategory;
        const std::string_view mName;

        explicit BaseSettingValue(
            SettingValueType type, std::string_view category, std::string_view name, Index& index);

        BaseSettingValue(const BaseSettingValue& other) = delete;

        BaseSettingValue(BaseSettingValue&& other);

        BaseSettingValue& operator=(const BaseSettingValue& other) = delete;

        BaseSettingValue& operator=(BaseSettingValue&& other) = delete;

    private:
        Index& mIndex;
    };

    template <class T>
    class SettingValue;

    class Index
    {
    public:
        template <class T>
        SettingValue<T>* find(std::string_view category, std::string_view name) const;

        template <class T>
        SettingValue<T>& get(std::string_view category, std::string_view name) const;

        void insert(BaseSettingValue* value)
        {
            if (!mValues.emplace(std::make_pair(value->mCategory, value->mName), value).second)
                throw std::invalid_argument("Duplicated setting definition: "
                    + getSettingDescription(value->mType, value->mCategory, value->mName));
        }

        void insertOrAssign(BaseSettingValue* value)
        {
            mValues.insert_or_assign(std::make_pair(value->mCategory, value->mName), value);
        }

    private:
        std::map<std::pair<std::string_view, std::string_view>, BaseSettingValue*> mValues;
    };

    inline BaseSettingValue::BaseSettingValue(
        SettingValueType type, std::string_view category, std::string_view name, Index& index)
        : mType(type)
        , mCategory(category)
        , mName(name)
        , mIndex(index)
    {
        mIndex.insert(this);
    }

    inline BaseSettingValue::BaseSettingValue(BaseSettingValue&& other)
        : mType(other.mType)
        , mCategory(other.mCategory)
        , mName(other.mName)
        , mIndex(other.mIndex)
    {
        mIndex.insertOrAssign(this);
    }

    template <class T>
    class SettingValue final : public BaseSettingValue
    {
    public:
        explicit SettingValue(Index& index, std::string_view category, std::string_view name,
            std::unique_ptr<const Sanitizer<T>>&& sanitizer = nullptr)
            : BaseSettingValue(getSettingValueType<T>(), category, name, index)
            , mSanitizer(std::move(sanitizer))
            , mValue(sanitize(Settings::Manager::get<T>(mName, mCategory)))
        {
        }

        SettingValue(SettingValue&& other)
            : BaseSettingValue(std::move(other))
            , mSanitizer(std::move(other.mSanitizer))
            , mDefaultValue(std::move(other.mValue))
            , mValue(sanitize(Settings::Manager::get<T>(mName, mCategory)))
        {
        }

        SettingValue(const SettingValue& other) = delete;

        SettingValue& operator=(const SettingValue& other) = delete;

        SettingValue& operator=(SettingValue&& other) = delete;

        const T& get() const { return mValue; }

        operator const T&() const { return mValue; }

        void set(const T& value)
        {
            if (mValue == value)
                return;
            mValue = sanitize(value);
            Settings::Manager::set(mName, mCategory, mValue);
        }

        void reset() { set(mDefaultValue); }

    private:
        std::unique_ptr<const Sanitizer<T>> mSanitizer;
        T mDefaultValue{};
        T mValue{};

        T sanitize(const T& value) const
        {
            if (mSanitizer == nullptr)
                return value;
            try
            {
                T sanitizedValue = mSanitizer->apply(value);
                if (sanitizedValue != value)
                    Log(Debug::Warning) << getSettingDescription<T>(mCategory, mName)
                                        << " value is out of allowed values set: " << value << ", sanitized to "
                                        << sanitizedValue;
                return sanitizedValue;
            }
            catch (const std::exception& e)
            {
                throw std::runtime_error(
                    "Invalid " + getSettingDescription<T>(mCategory, mName) + " value: " + std::string(e.what()));
            }
        }
    };

    template <class T>
    SettingValue<T>* Index::find(std::string_view category, std::string_view name) const
    {
        const auto it = mValues.find({ category, name });
        if (it == mValues.end())
            return nullptr;
        BaseSettingValue* const value = it->second;
        if (value->mType != getSettingValueType<T>())
            throw std::invalid_argument(getSettingDescription(value->mType, category, name)
                + " does not match requested type: " + std::string(getSettingValueTypeName<T>()));
        return static_cast<SettingValue<T>*>(value);
    }

    template <class T>
    SettingValue<T>& Index::get(std::string_view category, std::string_view name) const
    {
        SettingValue<T>* const result = find<T>(category, name);
        if (result == nullptr)
            throw std::invalid_argument(getSettingDescription<T>(category, name) + " is not found");
        return *result;
    }

    class WithIndex
    {
    public:
        explicit WithIndex(Index& index)
            : mIndex(index)
        {
        }

    protected:
        Index& mIndex;
    };
}

#endif
