#ifndef GAME_MWWORLD_REFDATA_H
#define GAME_MWWORLD_REFDATA_H

#include <components/esm/position.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/animationstate.hpp>

#include "../mwscript/locals.hpp"
#include "../mwworld/customdata.hpp"

#include <osg/ref_ptr>

#include <memory>
#include <string>

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

namespace ESM4
{
    struct ActorCharacter;
    struct Reference;
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
        osg::ref_ptr<SceneUtil::PositionAttitudeTransform> mBaseNode;

        MWScript::Locals mLocals;
        std::shared_ptr<MWLua::LocalScripts> mLuaScripts;
        ESM::Position mPosition;
        ESM::AnimationState mAnimationState;
        std::unique_ptr<CustomData> mCustomData;
        unsigned int mFlags;

        /// separate delete flag used for deletion by a content file
        /// @note not stored in the save game file.
        bool mDeletedByContentFile : 1;

        bool mEnabled : 1;

    public:
        bool mPhysicsPostponed : 1;

    private:
        bool mChanged : 1;

        void copy(const RefData& refData);

        void cleanup();

    public:
        RefData();

        /// @param cellRef Used to copy constant data such as position into this class where it can
        /// be altered without affecting the original data. This makes it possible
        /// to reset the position as the original data is still held in the CellRef
        RefData(const ESM::CellRef& cellRef);
        RefData(const ESM4::Reference& cellRef);
        RefData(const ESM4::ActorCharacter& cellRef);

        RefData(const ESM::ObjectState& objectState, bool deletedByContentFile);
        ///< Ignores local variables and custom data (not enough context available here to
        /// perform these operations).

        RefData(const RefData& refData);
        RefData(RefData&& other);

        ~RefData();

        void write(ESM::ObjectState& objectState, const ESM::RefId& scriptId = ESM::RefId()) const;
        ///< Ignores custom data (not enough context available here to
        /// perform this operations).

        RefData& operator=(const RefData& refData);
        RefData& operator=(RefData&& other);

        /// Return base node (can be a null pointer).
        SceneUtil::PositionAttitudeTransform* getBaseNode();

        /// Return base node (can be a null pointer).
        const SceneUtil::PositionAttitudeTransform* getBaseNode() const;

        /// Set base node (can be a null pointer).
        void setBaseNode(osg::ref_ptr<SceneUtil::PositionAttitudeTransform> base);

        void setLocals(const ESM::Script& script);

        MWLua::LocalScripts* getLuaScripts() const { return mLuaScripts.get(); }
        void setLuaScripts(std::shared_ptr<MWLua::LocalScripts>&&);

        /// This flag is only used for content stack loading and will not be stored in the savegame.
        /// If the object was deleted by gameplay, then use setCount(0) instead.
        void setDeletedByContentFile(bool deleted);

        /// Returns true if the object was deleted by a content file.
        bool isDeletedByContentFile() const;

        MWScript::Locals& getLocals();

        bool isEnabled() const;

        void enable();

        void disable();

        void setPosition(const ESM::Position& pos);
        const ESM::Position& getPosition() const;

        void setCustomData(std::unique_ptr<CustomData>&& value) noexcept;
        ///< Set custom data (potentially replacing old custom data). The ownership of \a data is
        /// transferred to this.

        CustomData* getCustomData();
        ///< May return a 0-pointer. The ownership of the return data object is not transferred.

        const CustomData* getCustomData() const;

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
