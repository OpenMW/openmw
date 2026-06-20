#include "environment.hpp"

#include <cassert>

#include <components/resource/resourcesystem.hpp>

#include "luamanager.hpp"

MWBase::Environment* MWBase::Environment::sThis = nullptr;

MWBase::Environment::Environment()
{
    assert(sThis == nullptr);
    sThis = this;
}

MWBase::Environment::~Environment()
{
    sThis = nullptr;
}

Misc::NotNullPtr<MWBase::LuaManager> MWBase::Environment::getLuaManagerForGlobalScripts() const
{
    if (mAuthoritativeLuaManager)
        return &mAuthoritativeLuaManager->get();
    return getLuaManager();
}

void MWBase::Environment::forEachLuaManagerAuthoritativeFirst(
    const std::function<void(LuaManager&)>& callback) const
{
    LuaManager* authoritative = mAuthoritativeLuaManager ? &mAuthoritativeLuaManager->get() : nullptr;
    LuaManager* client = mClientLuaManager ? &mClientLuaManager->get() : nullptr;
    LuaManager* fallback = mLuaManager ? &mLuaManager->get() : nullptr;

    if (authoritative != nullptr)
        callback(*authoritative);
    if (client != nullptr && client != authoritative)
        callback(*client);
    if (fallback != nullptr && fallback != authoritative && fallback != client)
        callback(*fallback);
}

void MWBase::Environment::reloadAllLuaManagersAuthoritativeFirst() const
{
    forEachLuaManagerAuthoritativeFirst([](LuaManager& luaManager) { luaManager.reloadAllScripts(); });
}
