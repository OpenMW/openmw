#include "debugbindings.hpp"
#include "context.hpp"
#include "luamanagerimp.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/postprocessor.hpp"

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/shader/shadermanager.hpp>

#include <components/lua/luastate.hpp>

namespace MWLua
{
    sol::table initDebugPackage(const Context& context)
    {
        sol::table api = context.mLua->newTable();

        api["RENDER_MODE"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, MWRender::RenderMode>({
            {"CollisionDebug", MWRender::Render_CollisionDebug},
            {"Wireframe", MWRender::Render_Wireframe},
            {"Pathgrid", MWRender::Render_Pathgrid},
            {"Water", MWRender::Render_Water},
            {"Scene", MWRender::Render_Scene},
            {"NavMesh", MWRender::Render_NavMesh},
            {"ActorsPaths", MWRender::Render_ActorsPaths},
            {"RecastMesh", MWRender::Render_RecastMesh},
        }));

        api["toggleRenderMode"] = [context] (MWRender::RenderMode value)
        {
            context.mLuaManager->addAction([value]
            {
                MWBase::Environment::get().getWorld()->toggleRenderMode(value);
            });
        };

        api["NAV_MESH_RENDER_MODE"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, MWRender::NavMeshMode>({
            {"AreaType", MWRender::NavMeshMode::AreaType},
            {"UpdateFrequency", MWRender::NavMeshMode::UpdateFrequency},
        }));

        api["setNavMeshRenderMode"] = [context] (MWRender::NavMeshMode value)
        {
            context.mLuaManager->addAction([value]
            {
                MWBase::Environment::get().getWorld()->getRenderingManager()->setNavMeshMode(value);
            });
        };

        api["triggerShaderReload"] = [context]()
        {
            context.mLuaManager->addAction([]
                {
                    auto world = MWBase::Environment::get().getWorld();

                    world->getRenderingManager()->getResourceSystem()->getSceneManager()->getShaderManager().triggerShaderReload();
                    world->getPostProcessor()->triggerShaderReload();
                });
        };

        api["setShaderHotReloadEnabled"] = [context](bool value)
        {
            context.mLuaManager->addAction([value]
                {
                    auto world = MWBase::Environment::get().getWorld();
                    world->getRenderingManager()->getResourceSystem()->getSceneManager()->getShaderManager().setHotReloadEnabled(value);
                    world->getPostProcessor()->mEnableLiveReload = value;
                });
        };

        return LuaUtil::makeReadOnly(api);
    }
}
