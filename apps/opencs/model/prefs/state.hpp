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

            const std::string mConfigFile;
            const Files::ConfigurationManager& mConfigurationManager;
            Settings::Manager mSettings;
            std::map<std::string, Category> mCategories;
            std::map<std::string, Category>::iterator mCurrentCategory;

            // not implemented
            State (const State&);
            State& operator= (const State&);

        private:

            void load();

            void declare();

            void declareCategory (const std::string& key, const std::string& name);

        public:

            State (const Files::ConfigurationManager& configurationManager);

            ~State();

            void save();

            /// \return collection of name, key pairs (sorted)
            std::vector<std::pair<std::string, std::string> > listCategories() const;

            static State& get();
    };

    // convenience function
    State& get();
}

#endif
