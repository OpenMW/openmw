#include "refdata.hpp"

#include <components/esm/objectstate.hpp>

#include "customdata.hpp"
#include "cellstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWWorld
{
    void RefData::copy (const RefData& refData)
    {
        mBaseNode = refData.mBaseNode;
        mLocals = refData.mLocals;
        mEnabled = refData.mEnabled;
        mCount = refData.mCount;
        mPosition = refData.mPosition;
        mChanged = refData.mChanged;
        mDeletedByContentFile = refData.mDeletedByContentFile;

        mCustomData = refData.mCustomData ? refData.mCustomData->clone() : 0;
    }

    void RefData::cleanup()
    {
        mBaseNode = 0;

        delete mCustomData;
        mCustomData = 0;
    }

    RefData::RefData()
    : mBaseNode(0), mDeletedByContentFile(false), mEnabled (true), mCount (1), mCustomData (0), mChanged(false)
    {
        for (int i=0; i<3; ++i)
        {
            mPosition.pos[i] = 0;
            mPosition.rot[i] = 0;
        }
    }

    RefData::RefData (const ESM::CellRef& cellRef)
    : mBaseNode(0), mDeletedByContentFile(false), mEnabled (true),
      mCount (1), mPosition (cellRef.mPos),
      mCustomData (0),
      mChanged(false) // Loading from ESM/ESP files -> assume unchanged
    {
    }

    RefData::RefData (const ESM::ObjectState& objectState, bool deletedByContentFile)
    : mBaseNode(0), mDeletedByContentFile(deletedByContentFile),
      mEnabled (objectState.mEnabled != 0),
      mCount (objectState.mCount),
      mPosition (objectState.mPosition),
      mCustomData (0),
      mChanged(true) // Loading from a savegame -> assume changed
    {
    }

    RefData::RefData (const RefData& refData)
    : mBaseNode(0), mCustomData (0)
    {
        try
        {
            copy (refData);
        }
        catch (...)
        {
            cleanup();
            throw;
        }
    }

    void RefData::write (ESM::ObjectState& objectState, const std::string& scriptId) const
    {
        objectState.mHasLocals = mLocals.write (objectState.mLocals, scriptId);

        objectState.mEnabled = mEnabled;
        objectState.mCount = mCount;
        objectState.mPosition = mPosition;
    }

    RefData& RefData::operator= (const RefData& refData)
    {
        try
        {
            cleanup();
            copy (refData);
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
        {}
    }

    void RefData::setBaseNode(SceneUtil::PositionAttitudeTransform *base)
    {
        mBaseNode = base;
    }

    SceneUtil::PositionAttitudeTransform* RefData::getBaseNode()
    {
        return mBaseNode;
    }

    const SceneUtil::PositionAttitudeTransform* RefData::getBaseNode() const
    {
        return mBaseNode;
    }

    int RefData::getCount() const
    {
        return mCount;
    }

    void RefData::setLocals (const ESM::Script& script)
    {
        if (mLocals.configure (script) && !mLocals.isEmpty())
            mChanged = true;
    }

    void RefData::setCount (int count)
    {
        if(count == 0)
            MWBase::Environment::get().getWorld()->removeRefScript(this);

        mChanged = true;

        mCount = count;
    }

    void RefData::setDeletedByContentFile(bool deleted)
    {
        mDeletedByContentFile = deleted;
    }

    bool RefData::isDeleted() const
    {
        return mDeletedByContentFile || mCount == 0;
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

    void RefData::setCustomData (CustomData *data)
    {
        mChanged = true; // We do not currently track CustomData, so assume anything with a CustomData is changed
        delete mCustomData;
        mCustomData = data;
    }

    CustomData *RefData::getCustomData()
    {
        return mCustomData;
    }

    const CustomData *RefData::getCustomData() const
    {
        return mCustomData;
    }

    bool RefData::hasChanged() const
    {
        return mChanged;
    }
}
