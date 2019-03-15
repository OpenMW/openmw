#include "actorspaths.hpp"
#include "vismask.hpp"

#include <components/sceneutil/agentpath.hpp>

#include <osg/PositionAttitudeTransform>

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
            const osg::Vec3f& halfExtents, const osg::Vec3f& start, const osg::Vec3f& end,
            const DetourNavigator::Settings& settings)
    {
        if (!mEnabled)
            return;

        const auto group = mGroups.find(actor);
        if (group != mGroups.end())
            mRootNode->removeChild(group->second);

        const auto newGroup = SceneUtil::createAgentPathGroup(path, halfExtents, start, end, settings);
        if (newGroup)
        {
            newGroup->setNodeMask(Mask_Debug);
            mRootNode->addChild(newGroup);
            mGroups[actor] = newGroup;
        }
    }

    void ActorsPaths::remove(const MWWorld::ConstPtr& actor)
    {
        const auto group = mGroups.find(actor);
        if (group != mGroups.end())
        {
            mRootNode->removeChild(group->second);
            mGroups.erase(group);
        }
    }

    void ActorsPaths::removeCell(const MWWorld::CellStore* const store)
    {
        for (auto it = mGroups.begin(); it != mGroups.end(); )
        {
            if (it->first.getCell() == store)
            {
                mRootNode->removeChild(it->second);
                it = mGroups.erase(it);
            }
            else
                ++it;
        }
    }

    void ActorsPaths::updatePtr(const MWWorld::ConstPtr& old, const MWWorld::ConstPtr& updated)
    {
        const auto it = mGroups.find(old);
        if (it == mGroups.end())
            return;
        auto group = std::move(it->second);
        mGroups.erase(it);
        mGroups.insert(std::make_pair(updated, std::move(group)));
    }

    void ActorsPaths::enable()
    {
        std::for_each(mGroups.begin(), mGroups.end(),
            [&] (const Groups::value_type& v) { mRootNode->addChild(v.second); });
        mEnabled = true;
    }

    void ActorsPaths::disable()
    {
        std::for_each(mGroups.begin(), mGroups.end(),
            [&] (const Groups::value_type& v) { mRootNode->removeChild(v.second); });
        mEnabled = false;
    }
}
