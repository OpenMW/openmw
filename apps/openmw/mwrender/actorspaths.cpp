#include "actorspaths.hpp"
#include "vismask.hpp"

#include <components/sceneutil/agentpath.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/detournavigator/settings.hpp>

#include <osg/PositionAttitudeTransform>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include <algorithm>

namespace MWRender
{
    ActorsPaths::ActorsPaths(const osg::ref_ptr<osg::Group>& root, bool enabled)
        : mRootNode(root)
        , mEnabled(enabled)
    {
    }

    ActorsPaths::~ActorsPaths()
    {
        if (mEnabled)
            disable();
    }

    bool ActorsPaths::toggle()
    {
        if (mEnabled)
            disable();
        else
            enable();

        return mEnabled;
    }

    void ActorsPaths::update(const MWWorld::ConstPtr& actor, const std::deque<osg::Vec3f>& path,
            const DetourNavigator::AgentBounds& agentBounds, const osg::Vec3f& start, const osg::Vec3f& end,
            const DetourNavigator::Settings& settings)
    {
        if (!mEnabled)
            return;

        const auto group = mGroups.find(actor.mRef);
        if (group != mGroups.end())
            mRootNode->removeChild(group->second.mNode);

        auto newGroup = SceneUtil::createAgentPathGroup(path, agentBounds, start, end, settings.mRecast);
        if (newGroup)
        {
            MWBase::Environment::get().getResourceSystem()->getSceneManager()->recreateShaders(newGroup, "debug");
            newGroup->setNodeMask(Mask_Debug);
            mRootNode->addChild(newGroup);
            mGroups[actor.mRef] = Group {actor.mCell, std::move(newGroup)};
        }
    }

    void ActorsPaths::remove(const MWWorld::ConstPtr& actor)
    {
        const auto group = mGroups.find(actor.mRef);
        if (group != mGroups.end())
        {
            mRootNode->removeChild(group->second.mNode);
            mGroups.erase(group);
        }
    }

    void ActorsPaths::removeCell(const MWWorld::CellStore* const store)
    {
        for (auto it = mGroups.begin(); it != mGroups.end(); )
        {
            if (it->second.mCell == store)
            {
                mRootNode->removeChild(it->second.mNode);
                it = mGroups.erase(it);
            }
            else
                ++it;
        }
    }

    void ActorsPaths::updatePtr(const MWWorld::ConstPtr& old, const MWWorld::ConstPtr& updated)
    {
        const auto it = mGroups.find(old.mRef);
        if (it == mGroups.end())
            return;
        it->second.mCell = updated.mCell;
    }

    void ActorsPaths::enable()
    {
        std::for_each(mGroups.begin(), mGroups.end(),
            [&] (const Groups::value_type& v) { mRootNode->addChild(v.second.mNode); });
        mEnabled = true;
    }

    void ActorsPaths::disable()
    {
        std::for_each(mGroups.begin(), mGroups.end(),
            [&] (const Groups::value_type& v) { mRootNode->removeChild(v.second.mNode); });
        mEnabled = false;
    }
}
