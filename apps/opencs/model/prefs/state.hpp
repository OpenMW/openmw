#ifndef CSV_PREFS_STATE_H
#define CSM_PREFS_STATE_H

#include <QObject>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

#include <components/settings/settings.hpp>

namespace CSMPrefs
{
    class State : public QObject
    {
            Q_OBJECT

            static State *sThis;

            const std::string mConfigFile;
            const Files::ConfigurationManager& mConfigurationManager;
            Settings::Manager mSettings;

            // not implemented
            State (const State&);
            State& operator= (const State&);

        private:

            void load();

            void declare();

        public:

            State (const Files::ConfigurationManager& configurationManager);

            ~State();

            void save();

            static State& get();
    };

    // convenience function
    State& get();
}

#endif
