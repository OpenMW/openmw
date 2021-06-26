#ifndef CSM_PREFS_STATE_H
#define CSM_PREFS_STATE_H

#include <map>
#include <string>

#include <QObject>
#include <QMutex>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

#include <components/settings/settings.hpp>

#include "category.hpp"
#include "setting.hpp"
#include "enumsetting.hpp"
#include "stringsetting.hpp"
#include "shortcutmanager.hpp"

class QColor;

namespace CSMPrefs
{
    class IntSetting;
    class DoubleSetting;
    class BoolSetting;
    class ColourSetting;
    class ShortcutSetting;
    class ModifierSetting;

    /// \brief User settings state
    ///
    /// \note Access to the user settings is thread-safe once all declarations and loading has
    /// been completed.
    class State : public QObject
    {
            Q_OBJECT

            static State *sThis;

        public:

            typedef std::map<std::string, Category> Collection;
            typedef Collection::iterator Iterator;

        private:

            const std::string mConfigFile;
            const std::string mDefaultConfigFile;
            const Files::ConfigurationManager& mConfigurationManager;
            ShortcutManager mShortcutManager;
            Settings::Manager mSettings;
            Collection mCategories;
            Iterator mCurrentCategory;
            QMutex mMutex;

            // not implemented
            State (const State&);
            State& operator= (const State&);

        private:

            void load();

            void declare();

            void declareCategory (const std::string& key);

            IntSetting& declareInt (const std::string& key, const std::string& label, int default_);
            DoubleSetting& declareDouble (const std::string& key, const std::string& label, double default_);

            BoolSetting& declareBool (const std::string& key, const std::string& label, bool default_);

            EnumSetting& declareEnum (const std::string& key, const std::string& label, EnumValue default_);

            ColourSetting& declareColour (const std::string& key, const std::string& label, QColor default_);

            ShortcutSetting& declareShortcut (const std::string& key, const std::string& label,
                const QKeySequence& default_);

            StringSetting& declareString (const std::string& key, const std::string& label, std::string default_);

            ModifierSetting& declareModifier(const std::string& key, const std::string& label, int modifier_);

            void declareSeparator();

            void declareSubcategory(const std::string& label);

            void setDefault (const std::string& key, const std::string& default_);

        public:

            State (const Files::ConfigurationManager& configurationManager);

            ~State();

            void save();

            Iterator begin();

            Iterator end();

            ShortcutManager& getShortcutManager();

            Category& operator[](const std::string& key);

            void update (const Setting& setting);

            static State& get();

            void resetCategory(const std::string& category);

            void resetAll();

        signals:

            void settingChanged (const CSMPrefs::Setting *setting);
    };

    // convenience function
    State& get();
}

#endif
