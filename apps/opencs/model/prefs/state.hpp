#ifndef CSV_PREFS_STATE_H
#define CSM_PREFS_STATE_H

#include <map>
#include <string>

#include <QObject>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

#include <components/settings/settings.hpp>

#include "category.hpp"

namespace CSMPrefs
{
    class State : public QObject
    {
            Q_OBJECT

            static State *sThis;

        public:

            typedef std::map<std::string, Category> Collection;
            typedef Collection::iterator Iterator;

        private:

            const std::string mConfigFile;
            const Files::ConfigurationManager& mConfigurationManager;
            Settings::Manager mSettings;
            Collection mCategories;
            Iterator mCurrentCategory;

            // not implemented
            State (const State&);
            State& operator= (const State&);

        private:

            void load();

            void declare();

            void declareCategory (const std::string& key);

        public:

            State (const Files::ConfigurationManager& configurationManager);

            ~State();

            void save();

            Iterator begin();

            Iterator end();

            Category& getCategory (const std::string& key);

            static State& get();
    };

    // convenience function
    State& get();
}

#endif
