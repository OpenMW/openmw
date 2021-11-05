#include "navigator.hpp"
#include "navigatorimpl.hpp"
#include "navigatorstub.hpp"
#include "recastglobalallocator.hpp"

namespace DetourNavigator
{
    std::unique_ptr<Navigator> makeNavigator(const Settings& settings)
    {
        DetourNavigator::RecastGlobalAllocator::init();
        return std::make_unique<NavigatorImpl>(settings);
    }

    std::unique_ptr<Navigator> makeNavigatorStub()
    {
        return std::make_unique<NavigatorStub>();
    }
}
