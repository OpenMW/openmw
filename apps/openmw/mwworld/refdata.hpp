#ifndef GAME_MWWORLD_REFDATA_H
#define GAME_MWWORLD_REFDATA_H

#include <components/esm/defs.hpp>

#include "../mwscript/locals.hpp"

namespace Ogre
{
    class SceneNode;
}

namespace ESM
{
    class Script;
    class CellRef;
    struct ObjectState;
}

namespace MWWorld
{
    struct LocalRotation{
        float rot[3];
    };

    class CustomData;

    class RefData
    {
            Ogre::SceneNode* mBaseNode;


            MWScript::Locals mLocals; // if we find the overhead of heaving a locals
                                      // object in the refdata of refs without a script,
                                      // we can make this a pointer later.
            bool mDeleted; // separate delete flag used for deletion by a content file
            bool mHasLocals;
            bool mEnabled;
            int mCount; // 0: deleted


            ESM::Position mPosition;

            LocalRotation mLocalRotation;

            CustomData *mCustomData;

            void copy (const RefData& refData);

            void cleanup();

            bool mChanged;

        public:

            RefData();

            /// @param cellRef Used to copy constant data such as position into this class where it can
            /// be altered without affecting the original data. This makes it possible
            /// to reset the position as the orignal data is still held in the CellRef
            RefData (const ESM::CellRef& cellRef);

            RefData (const ESM::ObjectState& objectState);
            ///< Ignores local variables and custom data (not enough context available here to
            /// perform these operations).

            RefData (const RefData& refData);

            ~RefData();

            void write (ESM::ObjectState& objectState, const std::string& scriptId = "") const;
            ///< Ignores custom data (not enough context available here to
            /// perform this operations).

            RefData& operator= (const RefData& refData);

            /// Return OGRE handle (may be empty).
            const std::string &getHandle();

            /// Return OGRE base node (can be a null pointer).
            Ogre::SceneNode* getBaseNode();

            /// Set OGRE base node (can be a null pointer).
            void setBaseNode (Ogre::SceneNode* base);

            int getCount() const;

            void setLocals (const ESM::Script& script);

            void setCount (int count);
            ///< Set object count (an object pile is a simple object with a count >1).
            ///
            /// \warning Do not call setCount() to add or remove objects from a
            /// container or an actor's inventory. Call ContainerStore::add() or
            /// ContainerStore::remove() instead.

            /// This flag is only used for content stack loading and will not be stored in the savegame.
            /// If the object was deleted by gameplay, then use setCount(0) instead.
            void setDeleted(bool deleted);

            /// Returns true if the object was either deleted by the content file or by gameplay.
            bool isDeleted() const;
            /// Returns true if the object was deleted by a content file.
            bool isDeletedByContentFile() const;

            MWScript::Locals& getLocals();

            bool isEnabled() const;

            void enable();

            void disable();

            void setPosition (const ESM::Position& pos);
            const ESM::Position& getPosition();

            void setLocalRotation (const LocalRotation& rotation);
            const LocalRotation& getLocalRotation();

            void setCustomData (CustomData *data);
            ///< Set custom data (potentially replacing old custom data). The ownership of \a data is
            /// transferred to this.

            CustomData *getCustomData();
            ///< May return a 0-pointer. The ownership of the return data object is not transferred.

            bool hasChanged() const;
            ///< Has this RefData changed since it was originally loaded?
    };
}

#endif
