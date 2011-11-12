#ifndef GAME_MWWORLD_REFDATA_H
#define GAME_MWWORLD_REFDATA_H

#include <string>

#include <boost/shared_ptr.hpp>

#include "../mwscript/locals.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/movement.hpp"

#include "containerstore.hpp"

namespace ESM
{
    class Script;
}

namespace MWWorld
{
    class RefData
    {
            std::string mHandle;

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
            /// @param cr Used to copy constant data such as position into this class where it can
            ///           be altered without effecting the original data. This makes it possible
            ///           to reset the position as the orignal data is still held in the CellRef
            RefData(const ESMS::CellRef& cr) : mHasLocals (false), mEnabled (true),
                                         mCount (1), mPosition(cr.pos) {}

            std::string getHandle()
            {
                return mHandle;
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

            void setHandle (const std::string& handle)
            {
                mHandle = handle;
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
