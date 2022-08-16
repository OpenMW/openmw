#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_GUARDEDNAVMESHCACHEITEM_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_GUARDEDNAVMESHCACHEITEM_H

namespace Misc
{
    template <class T>
    class ScopeGuarded;
}

namespace DetourNavigator
{
    class NavMeshCacheItem;

    using GuardedNavMeshCacheItem = Misc::ScopeGuarded<NavMeshCacheItem>;
}

#endif
