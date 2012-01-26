#ifndef GAME_MWWORLD_REFDATA_H
#define GAME_MWWORLD_REFDATA_H

#include <string>

#include <boost/shared_ptr.hpp>

#include <Ogre.h>

#include "../mwscript/locals.hpp"

#include "../mwmechanics/movement.hpp"

#include "containerstore.hpp"

namespace ESM
{
    class Script;
}

namespace MWWorld
{
    class CustomData;

    class RefData
    {
            Ogre::SceneNode* mBaseNode;


            MWScript::Locals mLocals; // if we find the overhead of heaving a locals
                                      // object in the refdata of refs without a script,
                                      // we can make this a pointer later.
            bool mHasLocals;
            bool mEnabled;
            int mCount; // 0: deleted

            ESM::Position mPosition;

            CustomData *mCustomData;

            // we are using shared pointer here to avoid having to create custom copy-constructor,
            // assignment operator and destructor. As a consequence though copying a RefData object
            // manually will probably give unexcepted results. This is not a problem since RefData
            // are never copied outside of container operations.
            boost::shared_ptr<MWMechanics::Movement> mMovement;

            boost::shared_ptr<ContainerStore<RefData> > mContainerStore;

            void copy (const RefData& refData);

            void cleanup();

        public:

            /// @param cellRef Used to copy constant data such as position into this class where it can
            /// be altered without effecting the original data. This makes it possible
            /// to reset the position as the orignal data is still held in the CellRef
            RefData (const ESMS::CellRef& cellRef);

            RefData (const RefData& refData);

            ~RefData();

            RefData& operator= (const RefData& refData);

            /// Return OGRE handle (may be empty).
            std::string getHandle();

            /// Return OGRE base node (can be a null pointer).
            Ogre::SceneNode* getBaseNode();

            /// Set OGRE base node (can be a null pointer).
            void setBaseNode (Ogre::SceneNode* base);

            int getCount() const;

            void setLocals (const ESM::Script& script);

            void setCount (int count);

            MWScript::Locals& getLocals();

            bool isEnabled() const;

            void enable();

            void disable();

            boost::shared_ptr<MWMechanics::Movement>& getMovement();

            boost::shared_ptr<ContainerStore<RefData> >& getContainerStore();

            ESM::Position& getPosition();

            void setCustomData (CustomData *data);
            ///< Set custom data (potentially replacing old custom data). The ownership of \æ data is
            /// transferred to this.

            CustomData *getCustomData();
            ///< May return a 0-pointer. The ownership of the return data object is not transferred.
    };
}

#endif
