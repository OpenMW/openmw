#include "recastmesh.hpp"

#include <components/sceneutil/recastmesh.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/detournavigator/settings.hpp>
#include <components/detournavigator/recastmesh.hpp>

#include <osg/PositionAttitudeTransform>

#include "vismask.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
namespace MWRender
{
    RecastMesh::RecastMesh(const osg::ref_ptr<osg::Group>& root, bool enabled)
        : mRootNode(root)
        , mEnabled(enabled)
    {
    }

    RecastMesh::~RecastMesh()
    {
        if (mEnabled)
            disable();
    }

    bool RecastMesh::toggle()
    {
        if (mEnabled)
            disable();
        else
            enable();

        return mEnabled;
    }

    void RecastMesh::update(const DetourNavigator::RecastMeshTiles& tiles, const DetourNavigator::Settings& settings)
    {
        if (!mEnabled)
            return;

        for (auto it = mGroups.begin(); it != mGroups.end();)
        {
            const auto tile = tiles.find(it->first);
            if (tile == tiles.end())
            {
                mRootNode->removeChild(it->second.mValue);
                it = mGroups.erase(it);
                continue;
            }

            if (it->second.mGeneration != tile->second->getGeneration()
                || it->second.mRevision != tile->second->getRevision())
            {
                const auto group = SceneUtil::createRecastMeshGroup(*tile->second, settings);
                MWBase::Environment::get().getResourceSystem()->getSceneManager()->recreateShaders(group, "debug");
                group->setNodeMask(Mask_Debug);
                mRootNode->removeChild(it->second.mValue);
                mRootNode->addChild(group);
                it->second.mValue = group;
                it->second.mGeneration = tile->second->getGeneration();
                it->second.mRevision = tile->second->getRevision();
            }

            ++it;
        }

        for (const auto& tile : tiles)
        {
            if (mGroups.count(tile.first))
                continue;
            const auto group = SceneUtil::createRecastMeshGroup(*tile.second, settings);
            MWBase::Environment::get().getResourceSystem()->getSceneManager()->recreateShaders(group, "debug");
            group->setNodeMask(Mask_Debug);
            mGroups.emplace(tile.first, Group {tile.second->getGeneration(), tile.second->getRevision(), group});
            mRootNode->addChild(group);
        }
    }

    void RecastMesh::reset()
    {
        std::for_each(mGroups.begin(), mGroups.end(), [&] (const auto& v) { mRootNode->removeChild(v.second.mValue); });
        mGroups.clear();
    }

    void RecastMesh::enable()
    {
        std::for_each(mGroups.begin(), mGroups.end(), [&] (const auto& v) { mRootNode->addChild(v.second.mValue); });
        mEnabled = true;
    }

    void RecastMesh::disable()
    {
        std::for_each(mGroups.begin(), mGroups.end(), [&] (const auto& v) { mRootNode->removeChild(v.second.mValue); });
        mEnabled = false;
    }
}
