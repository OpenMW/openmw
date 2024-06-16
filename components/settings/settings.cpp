#include "settings.hpp"
#include "parser.hpp"
#include "values.hpp"

#include <charconv>
#include <filesystem>
#include <sstream>
#include <system_error>

#if !(defined(_MSC_VER) && (_MSC_VER >= 1924)) && !(defined(__GNUC__) && __GNUC__ >= 11) || defined(__clang__)         \
    || defined(__apple_build_version__)

#include <cerrno>
#include <ios>
#include <locale>

#endif

#include <components/debug/debuglog.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/conversion.hpp>

namespace Settings
{
    namespace
    {
        template <class T>
        T parseNumberFromSetting(const std::string& value, std::string_view setting, std::string_view category)
        {
            T number{};

            const auto result = std::from_chars(value.data(), value.data() + value.size(), number);
            if (result.ec != std::errc())
            {
                throw std::system_error(std::make_error_code(result.ec),
                    "Failed to parse number from setting [" + std::string(category) + "] " + std::string(setting)
                        + " value \"" + value + "\"");
            }

            return number;
        }

#if !(defined(_MSC_VER) && (_MSC_VER >= 1924)) && !(defined(__GNUC__) && __GNUC__ >= 11) || defined(__clang__)         \
    || defined(__apple_build_version__)
        template <>
        float parseNumberFromSetting<float>(
            const std::string& value, std::string_view setting, std::string_view category)
        {
            std::istringstream iss(value);
            iss.imbue(std::locale::classic());

            float floatValue = 0.0f;

            if (!(iss >> floatValue))
            {
                throw std::system_error(errno, std::generic_category(),
                    "Failed to parse number from setting [" + std::string(category) + "] " + std::string(setting)
                        + " value \"" + value + "\"");
            }

            return floatValue;
        }

        template <>
        double parseNumberFromSetting<double>(
            const std::string& value, std::string_view setting, std::string_view category)
        {
            std::istringstream iss(value);
            iss.imbue(std::locale::classic());

            double doubleValue = 0.0;

            if (!(iss >> doubleValue))
            {
                throw std::system_error(errno, std::generic_category(),
                    "Failed to parse number from setting [" + std::string(category) + "] " + std::string(setting)
                        + " value \"" + value + "\"");
            }

            return doubleValue;
        }
#endif
        template <class T>
        std::string serialize(const T& value)
        {
            std::ostringstream stream;
            stream << value;
            return stream.str();
        }

        std::string toString(SceneUtil::LightingMethod value)
        {
            switch (value)
            {
                case SceneUtil::LightingMethod::FFP:
                    return "legacy";
                case SceneUtil::LightingMethod::PerObjectUniform:
                    return "shaders compatibility";
                case SceneUtil::LightingMethod::SingleUBO:
                    return "shaders";
            }

            throw std::invalid_argument("Invalid LightingMethod value: " + std::to_string(static_cast<int>(value)));
        }

        int toInt(HrtfMode value)
        {
            switch (value)
            {
                case HrtfMode::Auto:
                    return -1;
                case HrtfMode::Disable:
                    return 0;
                case HrtfMode::Enable:
                    return 1;
            }

            Log(Debug::Warning) << "Invalid HRTF mode value: " << static_cast<int>(value) << ", fallback to auto (-1)";
            return -1;
        }
    }

    CategorySettingValueMap Manager::mDefaultSettings = CategorySettingValueMap();
    CategorySettingValueMap Manager::mUserSettings = CategorySettingValueMap();
    CategorySettingVector Manager::mChangedSettings = CategorySettingVector();
    std::set<std::pair<std::string_view, std::string_view>> Manager::sInitialized;

    void Manager::clear()
    {
        sInitialized.clear();
        StaticValues::clear();
        mDefaultSettings.clear();
        mUserSettings.clear();
        mChangedSettings.clear();
    }

    std::filesystem::path Manager::load(const Files::ConfigurationManager& cfgMgr, bool loadEditorSettings)
    {
        SettingsFileParser parser;
        const std::vector<std::filesystem::path>& paths = cfgMgr.getActiveConfigPaths();
        if (paths.empty())
            throw std::runtime_error("No config dirs! ConfigurationManager::readConfiguration must be called first.");

        // Create file name strings for either the engine or the editor.
        std::string defaultSettingsFile;
        std::string userSettingsFile;

        if (!loadEditorSettings)
        {
            defaultSettingsFile = "defaults.bin";
            userSettingsFile = "settings.cfg";
        }
        else
        {
            defaultSettingsFile = "defaults-cs.bin";
            userSettingsFile = "openmw-cs.cfg";
        }

        // Create the settings manager and load default settings file.
        const auto defaultsBin = paths.front() / defaultSettingsFile;
        if (!std::filesystem::exists(defaultsBin))
            throw std::runtime_error("No default settings file found! Make sure the file \"" + defaultSettingsFile
                + "\" was properly installed.");
        parser.loadSettingsFile(defaultsBin, mDefaultSettings, true, false);

        const CategorySettingValueMap originalDefaultSettings = mDefaultSettings;

        // Load "settings.cfg" or "openmw-cs.cfg" from every config dir except the last one as additional default
        // settings.
        for (int i = 0; i < static_cast<int>(paths.size()) - 1; ++i)
        {
            const auto additionalDefaults = paths[i] / userSettingsFile;
            if (std::filesystem::exists(additionalDefaults))
                parser.loadSettingsFile(additionalDefaults, mDefaultSettings, false, true);
        }

        if (!loadEditorSettings)
            Settings::StaticValues::initDefaults();

        // Load "settings.cfg" or "openmw-cs.cfg" from the last config dir as user settings. This path will be used to
        // save modified settings.
        auto settingspath = paths.back() / userSettingsFile;
        if (std::filesystem::exists(settingspath))
            parser.loadSettingsFile(settingspath, mUserSettings, false, false);

        if (!loadEditorSettings)
            Settings::StaticValues::init();

        for (const auto& [key, value] : originalDefaultSettings)
            if (!sInitialized.contains(key))
                throw std::runtime_error("Default setting [" + key.first + "] " + key.second + " is not initialized");

        return settingspath;
    }

    void Manager::saveUser(const std::filesystem::path& file)
    {
        SettingsFileParser parser;
        parser.saveSettingsFile(file, mUserSettings);
    }

    const std::string& Manager::getString(std::string_view setting, std::string_view category)
    {
        const auto key = std::make_pair(category, setting);
        CategorySettingValueMap::iterator it = mUserSettings.find(key);
        if (it != mUserSettings.end())
            return it->second;

        it = mDefaultSettings.find(key);
        if (it != mDefaultSettings.end())
            return it->second;

        throw std::runtime_error("Trying to retrieve a non-existing setting: [" + std::string(category) + "] "
            + std::string(setting) + ".\nMake sure the defaults.bin file was properly installed.");
    }

    std::vector<std::string> Manager::getStringArray(std::string_view setting, std::string_view category)
    {
        // TODO: it is unclear how to handle empty value -
        // there is no difference between empty serialized array
        // and a serialized array which has one empty value
        std::vector<std::string> values;
        const std::string& value = getString(setting, category);
        if (value.empty())
            return values;

        Misc::StringUtils::split(value, values, ",");
        for (auto& item : values)
            Misc::StringUtils::trim(item);
        return values;
    }

    float Manager::getFloat(std::string_view setting, std::string_view category)
    {
        return parseNumberFromSetting<float>(getString(setting, category), setting, category);
    }

    double Manager::getDouble(std::string_view setting, std::string_view category)
    {
        return parseNumberFromSetting<double>(getString(setting, category), setting, category);
    }

    int Manager::getInt(std::string_view setting, std::string_view category)
    {
        return parseNumberFromSetting<int>(getString(setting, category), setting, category);
    }

    std::uint64_t Manager::getUInt64(std::string_view setting, std::string_view category)
    {
        return parseNumberFromSetting<uint64_t>(getString(setting, category), setting, category);
    }

    std::size_t Manager::getSize(std::string_view setting, std::string_view category)
    {
        return parseNumberFromSetting<size_t>(getString(setting, category), setting, category);
    }

    unsigned Manager::getUnsigned(std::string_view setting, std::string_view category)
    {
        return parseNumberFromSetting<unsigned>(getString(setting, category), setting, category);
    }

    unsigned long Manager::getUnsignedLong(std::string_view setting, std::string_view category)
    {
        return parseNumberFromSetting<unsigned long>(getString(setting, category), setting, category);
    }

    unsigned long long Manager::getUnsignedLongLong(std::string_view setting, std::string_view category)
    {
        return parseNumberFromSetting<unsigned long long>(getString(setting, category), setting, category);
    }

    bool Manager::getBool(std::string_view setting, std::string_view category)
    {
        const std::string& string = getString(setting, category);
        return Misc::StringUtils::ciEqual(string, "true");
    }

    osg::Vec2f Manager::getVector2(std::string_view setting, std::string_view category)
    {
        const std::string& value = getString(setting, category);

        std::vector<std::string> components;
        Misc::StringUtils::split(value, components);

        if (components.size() == 2)
        {
            auto x = Misc::StringUtils::toNumeric<float>(components[0]);
            auto y = Misc::StringUtils::toNumeric<float>(components[1]);

            if (x && y)
            {
                return { x.value(), y.value() };
            }
        }

        throw std::runtime_error(std::string("Can't parse 2d vector: " + value));
    }

    osg::Vec3f Manager::getVector3(std::string_view setting, std::string_view category)
    {
        const std::string& value = getString(setting, category);

        std::vector<std::string> components;
        Misc::StringUtils::split(value, components);

        if (components.size() == 3)
        {
            auto x = Misc::StringUtils::toNumeric<float>(components[0]);
            auto y = Misc::StringUtils::toNumeric<float>(components[1]);
            auto z = Misc::StringUtils::toNumeric<float>(components[2]);

            if (x && y && z)
            {
                return { x.value(), y.value(), z.value() };
            }
        }

        throw std::runtime_error(std::string("Can't parse 3d vector: " + value));
    }

    void Manager::setString(std::string_view setting, std::string_view category, const std::string& value)
    {
        auto found = mUserSettings.find(std::make_pair(category, setting));
        if (found != mUserSettings.end())
        {
            if (found->second == value)
                return;
        }

        CategorySettingValueMap::key_type key(category, setting);

        mUserSettings[key] = value;

        mChangedSettings.insert(std::move(key));
    }

    void Manager::setStringArray(
        std::string_view setting, std::string_view category, const std::vector<std::string>& value)
    {
        std::stringstream stream;

        // TODO: escape delimeters, new line characters, etc.
        for (size_t i = 0; i < value.size(); ++i)
        {
            std::string item = value[i];
            Misc::StringUtils::trim(item);
            stream << item;

            if (i < value.size() - 1)
                stream << ",";
        }

        setString(setting, category, stream.str());
    }

    void Manager::setInt(std::string_view setting, std::string_view category, const int value)
    {
        std::ostringstream stream;
        stream << value;
        setString(setting, category, stream.str());
    }

    void Manager::setUInt64(std::string_view setting, std::string_view category, const std::uint64_t value)
    {
        std::ostringstream stream;
        stream << value;
        setString(setting, category, stream.str());
    }

    void Manager::setFloat(std::string_view setting, std::string_view category, const float value)
    {
        std::ostringstream stream;
        stream << value;
        setString(setting, category, stream.str());
    }

    void Manager::setDouble(std::string_view setting, std::string_view category, const double value)
    {
        std::ostringstream stream;
        stream << value;
        setString(setting, category, stream.str());
    }

    void Manager::setBool(std::string_view setting, std::string_view category, const bool value)
    {
        setString(setting, category, value ? "true" : "false");
    }

    void Manager::setVector2(std::string_view setting, std::string_view category, const osg::Vec2f value)
    {
        std::ostringstream stream;
        stream << value.x() << " " << value.y();
        setString(setting, category, stream.str());
    }

    void Manager::setVector3(std::string_view setting, std::string_view category, const osg::Vec3f value)
    {
        std::ostringstream stream;
        stream << value.x() << ' ' << value.y() << ' ' << value.z();
        setString(setting, category, stream.str());
    }

    CategorySettingVector Manager::getPendingChanges()
    {
        return mChangedSettings;
    }

    CategorySettingVector Manager::getPendingChanges(const CategorySettingVector& filter)
    {
        CategorySettingVector intersection;
        std::set_intersection(mChangedSettings.begin(), mChangedSettings.end(), filter.begin(), filter.end(),
            std::inserter(intersection, intersection.begin()));
        return intersection;
    }

    void Manager::resetPendingChanges()
    {
        mChangedSettings.clear();
    }

    void Manager::resetPendingChanges(const CategorySettingVector& filter)
    {
        for (const auto& key : filter)
        {
            mChangedSettings.erase(key);
        }
    }

    void Manager::set(std::string_view setting, std::string_view category, int value)
    {
        setInt(setting, category, value);
    }

    void Manager::set(std::string_view setting, std::string_view category, unsigned value)
    {
        setString(setting, category, serialize(value));
    }

    void Manager::set(std::string_view setting, std::string_view category, unsigned long value)
    {
        setString(setting, category, serialize(value));
    }

    void Manager::set(std::string_view setting, std::string_view category, unsigned long long value)
    {
        setString(setting, category, serialize(value));
    }

    void Manager::set(std::string_view setting, std::string_view category, float value)
    {
        setFloat(setting, category, value);
    }

    void Manager::set(std::string_view setting, std::string_view category, double value)
    {
        setDouble(setting, category, value);
    }

    void Manager::set(std::string_view setting, std::string_view category, const std::string& value)
    {
        setString(setting, category, value);
    }

    void Manager::set(std::string_view setting, std::string_view category, bool value)
    {
        setBool(setting, category, value);
    }

    void Manager::set(std::string_view setting, std::string_view category, const osg::Vec2f& value)
    {
        setVector2(setting, category, value);
    }

    void Manager::set(std::string_view setting, std::string_view category, const osg::Vec3f& value)
    {
        setVector3(setting, category, value);
    }

    void Manager::set(std::string_view setting, std::string_view category, DetourNavigator::CollisionShapeType value)
    {
        setInt(setting, category, static_cast<int>(value));
    }

    void Manager::set(std::string_view setting, std::string_view category, const std::vector<std::string>& value)
    {
        setStringArray(setting, category, value);
    }

    void Manager::set(std::string_view setting, std::string_view category, const MyGUI::Colour& value)
    {
        setString(setting, category, value.print());
    }

    void Manager::set(std::string_view setting, std::string_view category, SceneUtil::LightingMethod value)
    {
        setString(setting, category, toString(value));
    }

    void Manager::set(std::string_view setting, std::string_view category, HrtfMode value)
    {
        setInt(setting, category, toInt(value));
    }

    void Manager::set(std::string_view setting, std::string_view category, WindowMode value)
    {
        setInt(setting, category, static_cast<int>(value));
    }

    void Manager::set(std::string_view setting, std::string_view category, SDLUtil::VSyncMode value)
    {
        setInt(setting, category, static_cast<int>(value));
    }

    void Manager::recordInit(std::string_view setting, std::string_view category)
    {
        sInitialized.emplace(category, setting);
    }

    GyroscopeAxis parseGyroscopeAxis(std::string_view value)
    {
        if (value == "x")
            return GyroscopeAxis::X;
        else if (value == "y")
            return GyroscopeAxis::Y;
        else if (value == "z")
            return GyroscopeAxis::Z;
        else if (value == "-x")
            return GyroscopeAxis::MinusX;
        else if (value == "-y")
            return GyroscopeAxis::MinusY;
        else if (value == "-z")
            return GyroscopeAxis::MinusZ;

        throw std::runtime_error("Invalid gyroscope axis: " + std::string(value));
    }

    NavMeshRenderMode parseNavMeshRenderMode(std::string_view value)
    {
        if (value == "area type")
            return NavMeshRenderMode::AreaType;
        if (value == "update frequency")
            return NavMeshRenderMode::UpdateFrequency;

        throw std::invalid_argument("Invalid navigation mesh rendering mode: " + std::string(value));
    }

    SceneUtil::LightingMethod parseLightingMethod(std::string_view value)
    {
        if (value == "legacy")
            return SceneUtil::LightingMethod::FFP;
        if (value == "shaders compatibility")
            return SceneUtil::LightingMethod::PerObjectUniform;
        if (value == "shaders")
            return SceneUtil::LightingMethod::SingleUBO;

        constexpr const char* fallback = "shaders compatibility";
        Log(Debug::Warning) << "Unknown lighting method '" << value << "', returning fallback '" << fallback << "'";
        return SceneUtil::LightingMethod::PerObjectUniform;
    }
}
