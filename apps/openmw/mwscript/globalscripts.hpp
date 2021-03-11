#ifndef GAME_SCRIPT_GLOBALSCRIPTS_H
#define GAME_SCRIPT_GLOBALSCRIPTS_H

#include <boost/variant/variant.hpp>

#include <string>
#include <map>
#include <memory>
#include <utility>

#include <stdint.h>

#include "locals.hpp"

#include "../mwworld/ptr.hpp"

namespace ESM
{
    class ESMWriter;
    class ESMReader;
    struct RefNum;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class ESMStore;
}

namespace MWScript
{
    struct GlobalScriptDesc
    {
        bool mRunning;
        Locals mLocals;
        boost::variant<MWWorld::Ptr, std::pair<ESM::RefNum, std::string> > mTarget; // Used to start targeted script

        GlobalScriptDesc();

        const MWWorld::Ptr* getPtrIfPresent() const; // Returns a Ptr if one has been resolved

        MWWorld::Ptr getPtr(); // Resolves mTarget to a Ptr and caches the (potentially empty) result

        std::string getId() const; // Returns the target's ID -- if any
    };

    class GlobalScripts
    {
            const MWWorld::ESMStore& mStore;
            std::map<std::string, std::shared_ptr<GlobalScriptDesc> > mScripts;

        public:

            GlobalScripts (const MWWorld::ESMStore& store);

            void addScript (const std::string& name, const MWWorld::Ptr& target = MWWorld::Ptr());

            void removeScript (const std::string& name);

            bool isRunning (const std::string& name) const;

            void run();
            ///< run all active global scripts

            void clear();

            void addStartup();
            ///< Add startup script

            int countSavedGameRecords() const;

            void write (ESM::ESMWriter& writer, Loading::Listener& progress) const;

            bool readRecord (ESM::ESMReader& reader, uint32_t type, const std::map<int, int>& contentFileMap);
            ///< Records for variables that do not exist are dropped silently.
            ///
            /// \return Known type?

            Locals& getLocals (const std::string& name);
            ///< If the script \a name has not been added as a global script yet, it is added
            /// automatically, but is not set to running state.

            const Locals* getLocalsIfPresent (const std::string& name) const;

            void updatePtrs(const MWWorld::Ptr& base, const MWWorld::Ptr& updated);
            ///< Update the Ptrs stored in mTarget. Should be called after the reference has been moved to a new cell.
    };
}

#endif
