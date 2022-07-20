#ifndef GAME_MWWORLD_REFDATA_H
#define GAME_MWWORLD_REFDATA_H

#include <components/esm/defs.hpp>
#include <components/esm3/animationstate.hpp>

#include "../mwscript/locals.hpp"
#include "../mwworld/customdata.hpp"

#include <string>
#include <memory>

namespace SceneUtil
{
    class PositionAttitudeTransform;
}

namespace ESM
{
    class Script;
    class CellRef;
    struct ObjectState;
}

namespace MWLua
{
    class LocalScripts;
}

namespace MWWorld
{

    class CustomData;

    class RefData
    {
            SceneUtil::PositionAttitudeTransform* mBaseNode;

            MWScript::Locals mLocals;
            std::shared_ptr<MWLua::LocalScripts> mLuaScripts;

            /// separate delete flag used for deletion by a content file
            /// @note not stored in the save game file.
            bool mDeletedByContentFile:1;

            bool mEnabled:1;
        public:
            bool mPhysicsPostponed:1;
        private:

            /// 0: deleted
            int mCount;

            ESM::Position mPosition;

            ESM::AnimationState mAnimationState;

            std::unique_ptr<CustomData> mCustomData;

            void copy (const RefData& refData);

            void cleanup();

            bool mChanged;

            unsigned int mFlags;

        public:
            RefData();

            /// @param cellRef Used to copy constant data such as position into this class where it can
            /// be altered without affecting the original data. This makes it possible
            /// to reset the position as the original data is still held in the CellRef
            RefData (const ESM::CellRef& cellRef);

            RefData (const ESM::ObjectState& objectState, bool deletedByContentFile);
            ///< Ignores local variables and custom data (not enough context available here to
            /// perform these operations).

            RefData (const RefData& refData);
            RefData (RefData&& other) noexcept;

            ~RefData();

            void write (ESM::ObjectState& objectState, const std::string& scriptId = "") const;
            ///< Ignores custom data (not enough context available here to
            /// perform this operations).

            RefData& operator= (const RefData& refData);
            RefData& operator= (RefData&& other) noexcept;

            /// Return base node (can be a null pointer).
            SceneUtil::PositionAttitudeTransform* getBaseNode();

            /// Return base node (can be a null pointer).
            const SceneUtil::PositionAttitudeTransform* getBaseNode() const;

            /// Set base node (can be a null pointer).
            void setBaseNode (SceneUtil::PositionAttitudeTransform* base);

            int getCount(bool absolute = true) const;

            void setLocals (const ESM::Script& script);

            MWLua::LocalScripts* getLuaScripts() { return mLuaScripts.get(); }
            void setLuaScripts(std::shared_ptr<MWLua::LocalScripts>&&);

            void setCount (int count);
            ///< Set object count (an object pile is a simple object with a count >1).
            ///
            /// \warning Do not call setCount() to add or remove objects from a
            /// container or an actor's inventory. Call ContainerStore::add() or
            /// ContainerStore::remove() instead.

            /// This flag is only used for content stack loading and will not be stored in the savegame.
            /// If the object was deleted by gameplay, then use setCount(0) instead.
            void setDeletedByContentFile(bool deleted);

            /// Returns true if the object was either deleted by the content file or by gameplay.
            bool isDeleted() const;
            /// Returns true if the object was deleted by a content file.
            bool isDeletedByContentFile() const;

            MWScript::Locals& getLocals();

            bool isEnabled() const;

            void enable();

            void disable();

            void setPosition (const ESM::Position& pos);
            const ESM::Position& getPosition() const;

            void setCustomData(std::unique_ptr<CustomData>&& value) noexcept;
            ///< Set custom data (potentially replacing old custom data). The ownership of \a data is
            /// transferred to this.

            CustomData *getCustomData();
            ///< May return a 0-pointer. The ownership of the return data object is not transferred.

            const CustomData *getCustomData() const;

            bool activate();

            bool onActivate();

            bool activateByScript();

            bool hasChanged() const;
            ///< Has this RefData changed since it was originally loaded?

            const ESM::AnimationState& getAnimationState() const;
            ESM::AnimationState& getAnimationState();
    };
}

#endif
