#ifndef GAME_MWWORLD_REFDATA_H
#define GAME_MWWORLD_REFDATA_H

#include <string>

#include <boost/shared_ptr.hpp>

#include "../mwscript/locals.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/movement.hpp"

#include "containerstore.hpp"
#include <Ogre.h>

namespace ESM
{
    class Script;
}

namespace MWWorld
{
    class RefData
    {
            Ogre::SceneNode* mBaseNode;


            MWScript::Locals mLocals; // if we find the overhead of heaving a locals
                                      // object in the refdata of refs without a script,
                                      // we can make this a pointer later.
            bool mHasLocals;
            bool mEnabled;
            int mCount; // 0: deleted

            // we are using shared pointer here to avoid having to create custom copy-constructor,
            // assignment operator and destructor. As a consequence though copying a RefData object
            // manually will probably give unexcepted results. This is not a problem since RefData
            // are never copied outside of container operations.
            boost::shared_ptr<MWMechanics::CreatureStats> mCreatureStats;
            boost::shared_ptr<MWMechanics::NpcStats> mNpcStats;
            boost::shared_ptr<MWMechanics::Movement> mMovement;

            boost::shared_ptr<ContainerStore<RefData> > mContainerStore;

            ESM::Position mPosition;


        public:
            RefData(const ESMS::CellRef& cr) : mHasLocals (false), mEnabled (true),
                                         mCount (1), mPosition(cr.pos), mBaseNode(0) {}


            std::string getHandle()
            {
                return mBaseNode->getName();
            }
            void setBaseNode(Ogre::SceneNode* base){
                 mBaseNode = base;
            }

            int getCount() const
            {
                return mCount;
            }

            void setLocals (const ESM::Script& script)
            {
                if (!mHasLocals)
                {
                    mLocals.configure (script);
                    mHasLocals = true;
                }
            }


            void setCount (int count)
            {
                mCount = count;
            }

            MWScript::Locals& getLocals()
            {
                return mLocals;
            }

            bool isEnabled() const
            {
                return mEnabled;
            }

            void enable()
            {
                mEnabled = true;
            }

            void disable()
            {
                mEnabled = true;
            }

            boost::shared_ptr<MWMechanics::CreatureStats>& getCreatureStats()
            {
                return mCreatureStats;
            }

            boost::shared_ptr<MWMechanics::NpcStats>& getNpcStats()
            {
                return mNpcStats;
            }

            boost::shared_ptr<MWMechanics::Movement>& getMovement()
            {
                return mMovement;
            }

            boost::shared_ptr<ContainerStore<RefData> >& getContainerStore()
            {
                return mContainerStore;
            }

            ESM::Position& getPosition()
            {
                return mPosition;
            }
    };
}

#endif
