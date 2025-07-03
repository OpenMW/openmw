#ifndef GAME_SCRIPT_GLOBALSCRIPTS_H
#define GAME_SCRIPT_GLOBALSCRIPTS_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

#include <components/esm/refid.hpp>
#include <components/misc/algorithm.hpp>

#include "locals.hpp"

#include "../mwworld/ptr.hpp"

namespace ESM
{
    class ESMWriter;
    class ESMReader;
    struct FormId;
    using RefNum = FormId;
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
        std::variant<MWWorld::Ptr, std::pair<ESM::RefNum, ESM::RefId>> mTarget; // Used to start targeted script

        GlobalScriptDesc();

        const MWWorld::Ptr* getPtrIfPresent() const; // Returns a Ptr if one has been resolved

        MWWorld::Ptr getPtr(); // Resolves mTarget to a Ptr and caches the (potentially empty) result

        ESM::RefId getId() const; // Returns the target's ID -- if any
    };

    class GlobalScripts
    {
        const MWWorld::ESMStore& mStore;
        std::unordered_map<ESM::RefId, std::shared_ptr<GlobalScriptDesc>> mScripts;

    public:
        GlobalScripts(const MWWorld::ESMStore& store);

        void addScript(const ESM::RefId& name, const MWWorld::Ptr& target = MWWorld::Ptr());

        void removeScript(const ESM::RefId& name);

        const std::unordered_map<ESM::RefId, std::shared_ptr<GlobalScriptDesc>>& getScripts() const { return mScripts; }

        bool isRunning(const ESM::RefId& name) const;

        void run();
        ///< run all active global scripts

        void clear();

        void addStartup();
        ///< Add startup script

        int countSavedGameRecords() const;

        void write(ESM::ESMWriter& writer, Loading::Listener& progress) const;

        bool readRecord(ESM::ESMReader& reader, uint32_t type);
        ///< Records for variables that do not exist are dropped silently.
        ///
        /// \return Known type?

        Locals& getLocals(const ESM::RefId& name);
        ///< If the script \a name has not been added as a global script yet, it is added
        /// automatically, but is not set to running state.

        const GlobalScriptDesc* getScriptIfPresent(const ESM::RefId& name) const;

        void updatePtrs(const MWWorld::Ptr& base, const MWWorld::Ptr& updated);
        ///< Update the Ptrs stored in mTarget. Should be called after the reference has been moved to a new cell.
    };
}

#endif
