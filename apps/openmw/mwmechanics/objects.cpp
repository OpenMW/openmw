#include "objects.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadcont.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "character.hpp"

namespace MWMechanics
{

    void Objects::addObject(const MWWorld::Ptr& ptr)
    {
        removeObject(ptr);

        MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        if (anim == nullptr)
            return;

        const auto it = mObjects.emplace(mObjects.end(), ptr, *anim);
        mIndex.emplace(ptr.mRef, it);
    }

    void Objects::removeObject(const MWWorld::Ptr& ptr)
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
        {
            mObjects.erase(iter->second);
            mIndex.erase(iter);
        }
    }

    void Objects::updateObject(const MWWorld::Ptr& old, const MWWorld::Ptr& ptr)
    {
        const auto iter = mIndex.find(old.mRef);
        if (iter != mIndex.end())
            iter->second->updatePtr(ptr);
    }

    void Objects::dropObjects(const MWWorld::CellStore* cellStore)
    {
        for (auto iter = mObjects.begin(); iter != mObjects.end();)
        {
            if (iter->getPtr().getCell() == cellStore)
            {
                mIndex.erase(iter->getPtr().mRef);
                iter = mObjects.erase(iter);
            }
            else
                ++iter;
        }
    }

    void Objects::update(float duration, bool paused)
    {
        if (!paused)
        {
            for (CharacterController& object : mObjects)
                object.update(duration);
        }
        else
        {
            // We still should play container opening animation in the Container GUI mode.
            MWGui::GuiMode mode = MWBase::Environment::get().getWindowManager()->getMode();
            if (mode != MWGui::GM_Container)
                return;

            for (CharacterController& object : mObjects)
            {
                if (object.getPtr().getType() != ESM::Container::sRecordId)
                    continue;

                if (object.isAnimPlaying("containeropen"))
                {
                    object.update(duration);
                    MWBase::Environment::get().getWorld()->updateAnimatedCollisionShape(object.getPtr());
                }
            }
        }
    }

    bool Objects::onOpen(const MWWorld::Ptr& ptr)
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            return iter->second->onOpen();
        return true;
    }

    void Objects::onClose(const MWWorld::Ptr& ptr)
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            iter->second->onClose();
    }

    bool Objects::playAnimationGroup(
        const MWWorld::Ptr& ptr, std::string_view groupName, int mode, uint32_t number, bool scripted)
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
        {
            return iter->second->playGroup(groupName, mode, number, scripted);
        }
        else
        {
            Log(Debug::Warning) << "Warning: Objects::playAnimationGroup: Unable to find "
                                << ptr.getCellRef().getRefId();
            return false;
        }
    }

    bool Objects::playAnimationGroupLua(const MWWorld::Ptr& ptr, std::string_view groupName, uint32_t loops,
        float speed, std::string_view startKey, std::string_view stopKey, bool forceLoop)
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            return iter->second->playGroupLua(groupName, speed, startKey, stopKey, loops, forceLoop);
        return false;
    }

    void Objects::enableLuaAnimations(const MWWorld::Ptr& ptr, bool enable)
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            iter->second->enableLuaAnimations(enable);
    }

    void Objects::skipAnimation(const MWWorld::Ptr& ptr)
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            iter->second->skipAnim();
    }

    void Objects::persistAnimationStates()
    {
        for (CharacterController& object : mObjects)
            object.persistAnimationState();
    }

    void Objects::clearAnimationQueue(const MWWorld::Ptr& ptr, bool clearScripted)
    {
        const auto iter = mIndex.find(ptr.mRef);
        if (iter != mIndex.end())
            iter->second->clearAnimQueue(clearScripted);
    }

    void Objects::getObjectsInRange(const osg::Vec3f& position, float radius, std::vector<MWWorld::Ptr>& out) const
    {
        for (const CharacterController& object : mObjects)
            if ((position - object.getPtr().getRefData().getPosition().asVec3()).length2() <= radius * radius)
                out.push_back(object.getPtr());
    }

}
