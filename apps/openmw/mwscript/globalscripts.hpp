#ifndef GAME_SCRIPT_GLOBALSCRIPTS_H
#define GAME_SCRIPT_GLOBALSCRIPTS_H

#include <string>
#include <map>

#include <stdint.h>

#include "locals.hpp"

namespace ESM
{
    class ESMWriter;
    class ESMReader;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    struct ESMStore;
}

namespace MWScript
{
    class GlobalScripts
    {
            const MWWorld::ESMStore& mStore;
            std::map<std::string, std::pair<bool, Locals> > mScripts; // running, local variables

        public:

            GlobalScripts (const MWWorld::ESMStore& store);

            void addScript (const std::string& name);

            void removeScript (const std::string& name);

            bool isRunning (const std::string& name) const;

            void run();
            ///< run all active global scripts

            void clear();

            void addStartup();
            ///< Add startup script

            int countSavedGameRecords() const;

            void write (ESM::ESMWriter& writer, Loading::Listener& progress) const;

            bool readRecord (ESM::ESMReader& reader, int32_t type);
            ///< Records for variables that do not exist are dropped silently.
            ///
            /// \return Known type?

            Locals& getLocals (const std::string& name);
            ///< If the script \a name has not been added as a global script yet, it is added
            /// automatically, but is not set to running state.
    };
}

#endif
