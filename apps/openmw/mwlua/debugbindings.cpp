#include "debugbindings.hpp"

#include "context.hpp"
#include "luamanagerimp.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/postprocessor.hpp"
#include "../mwrender/renderingmanager.hpp"

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/shader/shadermanager.hpp>

#include <components/lua/luastate.hpp>

namespace MWLua
{
    sol::table initDebugPackage(const Context& context)
    {
        auto view = context.sol();
        sol::table api(view, sol::create);

        api["RENDER_MODE"]
            = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, MWRender::RenderMode>(view,
                {
                    { "CollisionDebug", MWRender::Render_CollisionDebug },
                    { "Wireframe", MWRender::Render_Wireframe },
                    { "Pathgrid", MWRender::Render_Pathgrid },
                    { "Water", MWRender::Render_Water },
                    { "Scene", MWRender::Render_Scene },
                    { "NavMesh", MWRender::Render_NavMesh },
                    { "ActorsPaths", MWRender::Render_ActorsPaths },
                    { "RecastMesh", MWRender::Render_RecastMesh },
                }));

        api["toggleRenderMode"] = [context](MWRender::RenderMode value) {
            context.mLuaManager->addAction([value] { MWBase::Environment::get().getWorld()->toggleRenderMode(value); });
        };

        api["toggleGodMode"] = []() { MWBase::Environment::get().getWorld()->toggleGodMode(); };
        api["isGodMode"] = []() { return MWBase::Environment::get().getWorld()->getGodModeState(); };

        api["toggleAI"] = []() { MWBase::Environment::get().getMechanicsManager()->toggleAI(); };
        api["isAIEnabled"] = []() { return MWBase::Environment::get().getMechanicsManager()->isAIActive(); };

        api["toggleCollision"] = []() { MWBase::Environment::get().getWorld()->toggleCollisionMode(); };
        api["isCollisionEnabled"] = []() {
            auto world = MWBase::Environment::get().getWorld();
            return world->isActorCollisionEnabled(world->getPlayerPtr());
        };

        api["toggleMWScript"] = []() { MWBase::Environment::get().getWorld()->toggleScripts(); };
        api["isMWScriptEnabled"] = []() { return MWBase::Environment::get().getWorld()->getScriptsEnabled(); };

        api["reloadLua"] = []() { MWBase::Environment::get().getLuaManager()->reloadAllScripts(); };

        api["NAV_MESH_RENDER_MODE"]
            = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, Settings::NavMeshRenderMode>(view,
                {
                    { "AreaType", Settings::NavMeshRenderMode::AreaType },
                    { "UpdateFrequency", Settings::NavMeshRenderMode::UpdateFrequency },
                }));

        api["setNavMeshRenderMode"] = [context](Settings::NavMeshRenderMode value) {
            context.mLuaManager->addAction(
                [value] { MWBase::Environment::get().getWorld()->getRenderingManager()->setNavMeshMode(value); });
        };

        api["triggerShaderReload"] = [context]() {
            context.mLuaManager->addAction([] {
                auto world = MWBase::Environment::get().getWorld();

                world->getRenderingManager()
                    ->getResourceSystem()
                    ->getSceneManager()
                    ->getShaderManager()
                    .triggerShaderReload();
                world->getPostProcessor()->triggerShaderReload();
            });
        };

        api["setShaderHotReloadEnabled"] = [context](bool value) {
            context.mLuaManager->addAction([value] {
                auto world = MWBase::Environment::get().getWorld();
                world->getRenderingManager()
                    ->getResourceSystem()
                    ->getSceneManager()
                    ->getShaderManager()
                    .setHotReloadEnabled(value);
                world->getPostProcessor()->mEnableLiveReload = value;
            });
        };

        return LuaUtil::makeReadOnly(api);
    }
}
