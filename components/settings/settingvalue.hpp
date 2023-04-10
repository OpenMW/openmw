#ifndef OPENMW_COMPONENTS_SETTINGS_SETTINGVALUE_H
#define OPENMW_COMPONENTS_SETTINGS_SETTINGVALUE_H

#include "sanitizer.hpp"
#include "settings.hpp"

#include "components/debug/debuglog.hpp"

#include <osg/io_utils>

#include <memory>
#include <stdexcept>
#include <string_view>

namespace Settings
{
    template <class T>
    class SettingValue
    {
    public:
        explicit SettingValue(
            std::string_view category, std::string_view name, std::unique_ptr<const Sanitizer<T>>&& sanitizer = nullptr)
            : mCategory(category)
            , mName(name)
            , mSanitizer(std::move(sanitizer))
            , mValue(sanitize(Settings::Manager::get<T>(name, category)))
        {
        }

        const T& get() const { return mValue; }

        operator const T&() const { return mValue; }

        void set(const T& value)
        {
            if (mValue == value)
                return;
            mValue = sanitize(value);
            Settings::Manager::set(mName, mCategory, mValue);
        }

    private:
        const std::string_view mCategory;
        const std::string_view mName;
        const std::unique_ptr<const Sanitizer<T>> mSanitizer;
        T mValue{};

        T sanitize(const T& value) const
        {
            if (mSanitizer == nullptr)
                return value;
            try
            {
                T sanitizedValue = mSanitizer->apply(value);
                if (sanitizedValue != value)
                    Log(Debug::Warning) << "Setting [" << mCategory << "] " << mName
                                        << " value is out of allowed values set: " << value << ", sanitized to "
                                        << sanitizedValue;
                return sanitizedValue;
            }
            catch (const std::exception& e)
            {
                throw std::runtime_error("Invalid setting [" + std::string(mCategory) + "] " + std::string(mName)
                    + " value: " + std::string(e.what()));
            }
        }
    };
}

#endif
