
#include "actors.hpp"

#include "../mwworld/class.hpp"

namespace MWMechanics
{
    Actors::Actors (MWWorld::Environment& environment) : mEnvironment (environment) {}

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

    void Actors::update (std::vector<std::pair<std::string, Ogre::Vector3> >& movement)
    {
        for (std::set<MWWorld::Ptr>::iterator iter (mActors.begin()); iter!=mActors.end();
            ++iter)
        {
            Ogre::Vector3 vector = MWWorld::Class::get (*iter).getMovementVector (*iter);

            if (vector!=Ogre::Vector3::ZERO)
                movement.push_back (std::make_pair (iter->getRefData().getHandle(), vector));
        }
    }
}
