#include "refdata.hpp"

#include <components/esm3/objectstate.hpp>
#include <components/esm4/loadachr.hpp>
#include <components/esm4/loadrefr.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>

#include "customdata.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwlua/localscripts.hpp"

namespace
{
    enum RefDataFlags
    {
        Flag_SuppressActivate = 1, // If set, activation will be suppressed and redirected to the OnActivate flag, which
                                   // can then be handled by a script.
        Flag_OnActivate = 2,
        Flag_ActivationBuffered = 4
    };
}

namespace MWWorld
{

    void RefData::setLuaScripts(std::shared_ptr<MWLua::LocalScripts>&& scripts)
    {
        mChanged = true;
        mLuaScripts = std::move(scripts);
    }

    void RefData::copy(const RefData& refData)
    {
        mBaseNode = refData.mBaseNode;
        mLocals = refData.mLocals;
        mEnabled = refData.mEnabled;
        mPosition = refData.mPosition;
        mChanged = refData.mChanged;
        mDeletedByContentFile = refData.mDeletedByContentFile;
        mFlags = refData.mFlags;
        mPhysicsPostponed = refData.mPhysicsPostponed;

        mAnimationState = refData.mAnimationState;

        mCustomData = refData.mCustomData ? refData.mCustomData->clone() : nullptr;
        mLuaScripts = refData.mLuaScripts;
    }

    void RefData::cleanup()
    {
        mBaseNode = nullptr;
        mCustomData = nullptr;
        mLuaScripts = nullptr;
    }

    RefData::RefData()
        : mBaseNode(nullptr)
        , mCustomData(nullptr)
        , mFlags(0)
        , mDeletedByContentFile(false)
        , mEnabled(true)
        , mPhysicsPostponed(false)
        , mChanged(false)
    {
        for (int i = 0; i < 3; ++i)
        {
            mPosition.pos[i] = 0;
            mPosition.rot[i] = 0;
        }
    }

    RefData::RefData(const ESM::CellRef& cellRef)
        : mBaseNode(nullptr)
        , mPosition(cellRef.mPos)
        , mCustomData(nullptr)
        , mFlags(0) // Loading from ESM/ESP files -> assume unchanged
        , mDeletedByContentFile(false)
        , mEnabled(true)
        , mPhysicsPostponed(false)
        , mChanged(false)
    {
    }

    RefData::RefData(const ESM4::Reference& ref)
        : mBaseNode(nullptr)
        , mPosition(ref.mPos)
        , mCustomData(nullptr)
        , mFlags(0)
        , mDeletedByContentFile(ref.mFlags & ESM4::Rec_Deleted)
        , mEnabled(!(ref.mFlags & ESM4::Rec_Disabled))
        , mPhysicsPostponed(false)
        , mChanged(false)
    {
    }

    RefData::RefData(const ESM4::ActorCharacter& ref)
        : mBaseNode(nullptr)
        , mPosition(ref.mPos)
        , mCustomData(nullptr)
        , mFlags(0)
        , mDeletedByContentFile(ref.mFlags & ESM4::Rec_Deleted)
        , mEnabled(!(ref.mFlags & ESM4::Rec_Disabled))
        , mPhysicsPostponed(false)
        , mChanged(false)
    {
    }

    RefData::RefData(const ESM::ObjectState& objectState, bool deletedByContentFile)
        : mBaseNode(nullptr)
        , mPosition(objectState.mPosition)
        , mAnimationState(objectState.mAnimationState)
        , mCustomData(nullptr)
        , mFlags(objectState.mFlags) // Loading from a savegame -> assume changed
        , mDeletedByContentFile(deletedByContentFile)
        , mEnabled(objectState.mEnabled != 0)
        , mPhysicsPostponed(false)
        , mChanged(true)
    {
        // "Note that the ActivationFlag_UseEnabled is saved to the reference,
        // which will result in permanently suppressed activation if the reference script is removed.
        // This occurred when removing the animated containers mod, and the fix in MCP is to reset UseEnabled to true on
        // loading a game."
        mFlags &= (~Flag_SuppressActivate);
    }

    RefData::RefData(const RefData& refData)
        : mBaseNode(nullptr)
        , mCustomData(nullptr)
    {
        try
        {
            copy(refData);
            mFlags &= ~(Flag_SuppressActivate | Flag_OnActivate | Flag_ActivationBuffered);
        }
        catch (...)
        {
            cleanup();
            throw;
        }
    }

    void RefData::write(ESM::ObjectState& objectState, const ESM::RefId& scriptId) const
    {
        objectState.mHasLocals = mLocals.write(objectState.mLocals, scriptId);

        objectState.mEnabled = mEnabled;
        objectState.mPosition = mPosition;
        objectState.mFlags = mFlags;

        objectState.mAnimationState = mAnimationState;
    }

    RefData& RefData::operator=(const RefData& refData)
    {
        try
        {
            cleanup();
            copy(refData);
        }
        catch (...)
        {
            cleanup();
            throw;
        }

        return *this;
    }

    RefData::~RefData()
    {
        try
        {
            cleanup();
        }
        catch (...)
        {
        }
    }

    RefData::RefData(RefData&& other) = default;
    RefData& RefData::operator=(RefData&& other) = default;

    void RefData::setBaseNode(osg::ref_ptr<SceneUtil::PositionAttitudeTransform> base)
    {
        mBaseNode = std::move(base);
    }

    SceneUtil::PositionAttitudeTransform* RefData::getBaseNode()
    {
        return mBaseNode;
    }

    const SceneUtil::PositionAttitudeTransform* RefData::getBaseNode() const
    {
        return mBaseNode;
    }

    void RefData::setLocals(const ESM::Script& script)
    {
        if (mLocals.configure(script) && !mLocals.isEmpty())
            mChanged = true;
    }

    void RefData::setDeletedByContentFile(bool deleted)
    {
        mDeletedByContentFile = deleted;
    }

    bool RefData::isDeletedByContentFile() const
    {
        return mDeletedByContentFile;
    }

    MWScript::Locals& RefData::getLocals()
    {
        return mLocals;
    }

    bool RefData::isEnabled() const
    {
        return mEnabled;
    }

    void RefData::enable()
    {
        if (!mEnabled)
        {
            mChanged = true;
            mEnabled = true;
        }
    }

    void RefData::disable()
    {
        if (mEnabled)
        {
            mChanged = true;
            mEnabled = false;
        }
    }

    void RefData::setPosition(const ESM::Position& pos)
    {
        mChanged = true;
        mPosition = pos;
    }

    const ESM::Position& RefData::getPosition() const
    {
        return mPosition;
    }

    void RefData::setCustomData(std::unique_ptr<CustomData>&& value) noexcept
    {
        mChanged = true; // We do not currently track CustomData, so assume anything with a CustomData is changed
        mCustomData = std::move(value);
    }

    CustomData* RefData::getCustomData()
    {
        return mCustomData.get();
    }

    const CustomData* RefData::getCustomData() const
    {
        return mCustomData.get();
    }

    bool RefData::hasChanged() const
    {
        return mChanged || !mAnimationState.empty();
    }

    bool RefData::activateByScript()
    {
        bool ret = (mFlags & Flag_ActivationBuffered);
        mFlags &= ~(Flag_SuppressActivate | Flag_OnActivate);
        return ret;
    }

    bool RefData::activate()
    {
        if (mFlags & Flag_SuppressActivate)
        {
            mFlags |= Flag_OnActivate | Flag_ActivationBuffered;
            return false;
        }
        else
        {
            return true;
        }
    }

    bool RefData::onActivate()
    {
        bool ret = mFlags & Flag_OnActivate;
        mFlags |= Flag_SuppressActivate;
        mFlags &= (~Flag_OnActivate);
        return ret;
    }

    const ESM::AnimationState& RefData::getAnimationState() const
    {
        return mAnimationState;
    }

    ESM::AnimationState& RefData::getAnimationState()
    {
        return mAnimationState;
    }

}
