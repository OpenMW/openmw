#ifndef CSM_PREFS_STATE_H
#define CSM_PREFS_STATE_H

#include <map>
#include <string>

#include <QMutex>
#include <QObject>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

#include "category.hpp"
#include "enumsetting.hpp"
#include "shortcutmanager.hpp"

class QColor;

namespace Settings
{
    class Index;
}

namespace CSMPrefs
{
    class IntSetting;
    class DoubleSetting;
    class BoolSetting;
    class ColourSetting;
    class ShortcutSetting;
    class ModifierSetting;
    class Setting;
    class StringSetting;
    class EnumSettingValue;
    struct Values;

    /// \brief User settings state
    ///
    /// \note Access to the user settings is thread-safe once all declarations and loading has
    /// been completed.
    class State : public QObject
    {
        Q_OBJECT

        static State* sThis;

    public:
        typedef std::map<std::string, Category> Collection;
        typedef Collection::iterator Iterator;

    private:
        const std::string mConfigFile;
        const std::string mDefaultConfigFile;
        const Files::ConfigurationManager& mConfigurationManager;
        ShortcutManager mShortcutManager;
        Collection mCategories;
        Iterator mCurrentCategory;
        QMutex mMutex;
        std::unique_ptr<Settings::Index> mIndex;
        std::unique_ptr<Values> mValues;

        void declare();

        void declareCategory(const std::string& key);

        IntSetting& declareInt(Settings::SettingValue<int>& value, const QString& label);

        DoubleSetting& declareDouble(Settings::SettingValue<double>& value, const QString& label);

        BoolSetting& declareBool(Settings::SettingValue<bool>& value, const QString& label);

        EnumSetting& declareEnum(EnumSettingValue& value, const QString& label);

        ColourSetting& declareColour(Settings::SettingValue<std::string>& value, const QString& label);

        ShortcutSetting& declareShortcut(Settings::SettingValue<std::string>& value, const QString& label);

        StringSetting& declareString(Settings::SettingValue<std::string>& value, const QString& label);

        ModifierSetting& declareModifier(Settings::SettingValue<std::string>& value, const QString& label);

        void declareSubcategory(const QString& label);

    public:
        State(const Files::ConfigurationManager& configurationManager);

        State(const State&) = delete;

        ~State();

        State& operator=(const State&) = delete;

        void save();

        Iterator begin();

        Iterator end();

        ShortcutManager& getShortcutManager();

        Category& operator[](const std::string& key);

        void update(const Setting& setting);

        static State& get();

        void resetCategory(const std::string& category);

        void resetAll();

    signals:

        void settingChanged(const CSMPrefs::Setting* setting);
    };

    // convenience function
    State& get();
}

#endif
