#include "scripttracker.hpp"

namespace LuaUtil
{
    namespace
    {
        constexpr unsigned sMinLoadedFrames = 50;
        constexpr unsigned sMaxLoadedFrames = 600;
        constexpr unsigned sUsageFrameGrowth = 10;
        constexpr std::size_t sMinToProcess = 1;
        constexpr std::size_t sToProcessDiv = 20; // 5%
    }

    void ScriptTracker::onLoad(ScriptsContainer& container)
    {
        mLoadedScripts.emplace(container.mThis, sMinLoadedFrames + mFrame);
    }

    void ScriptTracker::unloadInactiveScripts(LuaView& lua)
    {
        // This code is technically incorrect if mFrame overflows... but at 300fps that takes about half a year
        std::size_t toProcess = std::max(mLoadedScripts.size() / sToProcessDiv, sMinToProcess);
        while (toProcess && !mLoadedScripts.empty())
        {
            --toProcess;
            auto [ptr, ttl] = std::move(mLoadedScripts.front());
            mLoadedScripts.pop();
            ScriptsContainer* container = *ptr.get();
            // Object no longer exists, cease tracking
            if (!container)
                continue;
            // Ignore activity of local scripts in the active grid
            if (container->isActive())
                ttl = std::max(ttl, mFrame + sMinLoadedFrames);
            else
            {
                bool activeSinceLastPop = container->mRequiredLoading;
                if (activeSinceLastPop)
                {
                    container->mRequiredLoading = false;
                    ttl = std::min(ttl + sUsageFrameGrowth, mFrame + sMaxLoadedFrames);
                }
                else if (ttl < mFrame)
                {
                    container->ensureUnloaded(lua);
                    continue;
                }
            }
            mLoadedScripts.emplace(std::move(ptr), ttl);
        }
        ++mFrame;
    }
}
