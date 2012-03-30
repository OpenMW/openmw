
#include "actors.hpp"

#include <typeinfo>

#include <components/esm/loadnpc.hpp>

#include "../mwworld/class.hpp"

namespace MWMechanics
{
    void Actors::updateActor (const MWWorld::Ptr& ptr, float duration)
    {

    }

    void Actors::updateNpc (const MWWorld::Ptr& ptr, float duration, bool paused)
    {

    }

    Actors::Actors (MWWorld::Environment& environment) : mEnvironment (environment), mDuration (0) {}

    void Actors::addActor (const MWWorld::Ptr& ptr)
    {
        mActors.insert (ptr);
    }

    void Actors::removeActor (const MWWorld::Ptr& ptr)
    {
        mActors.erase (ptr);
    }

    void Actors::dropActors (const MWWorld::Ptr::CellStore *cellStore)
    {
        std::set<MWWorld::Ptr>::iterator iter = mActors.begin();

        while (iter!=mActors.end())
            if (iter->getCell()==cellStore)
            {
                mActors.erase (iter++);
            }
            else
                ++iter;
    }

    void Actors::update (std::vector<std::pair<std::string, Ogre::Vector3> >& movement, float duration,
        bool paused)
    {
        mDuration += duration;

        if (mDuration>=0.25)
        {
            for (std::set<MWWorld::Ptr>::iterator iter (mActors.begin()); iter!=mActors.end(); ++iter)
            {
                updateActor (*iter, mDuration);

                if (iter->getTypeName()==typeid (ESM::NPC).name())
                    updateNpc (*iter, mDuration, paused);
            }

            mDuration = 0;
        }

        for (std::set<MWWorld::Ptr>::iterator iter (mActors.begin()); iter!=mActors.end();
            ++iter)
        {
            Ogre::Vector3 vector = MWWorld::Class::get (*iter).getMovementVector (*iter);

            if (vector!=Ogre::Vector3::ZERO)
                movement.push_back (std::make_pair (iter->getRefData().getHandle(), vector));
        }
    }
}
