
#include "refdata.hpp"

#include <OgreSceneNode.h>

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
        mHasLocals = refData.mHasLocals;
        mEnabled = refData.mEnabled;
        mCount = refData.mCount;
        mPosition = refData.mPosition;
        mLocalRotation = refData.mLocalRotation;

        mCustomData = refData.mCustomData ? refData.mCustomData->clone() : 0;
    }

    void RefData::cleanup()
    {
        mBaseNode = 0;

        delete mCustomData;
        mCustomData = 0;
    }

    RefData::RefData()
    : mBaseNode(0), mHasLocals (false), mEnabled (true), mCount (1), mCustomData (0)
    {
        for (int i=0; i<3; ++i)
        {
            mLocalRotation.rot[i] = 0;
            mPosition.pos[i] = 0;
            mPosition.rot[i] = 0;
        }
    }

    RefData::RefData (const ESM::CellRef& cellRef)
    : mBaseNode(0), mHasLocals (false), mEnabled (true), mCount (1), mPosition (cellRef.mPos),
      mCustomData (0)
    {
        mLocalRotation.rot[0]=0;
        mLocalRotation.rot[1]=0;
        mLocalRotation.rot[2]=0;
    }

    RefData::RefData (const ESM::ObjectState& objectState)
    : mBaseNode (0), mHasLocals (false), mEnabled (objectState.mEnabled),
      mCount (objectState.mCount), mPosition (objectState.mPosition), mCustomData (0)
    {
        for (int i=0; i<3; ++i)
            mLocalRotation.rot[i] = objectState.mLocalRotation[i];
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
        objectState.mHasLocals = mHasLocals;

        if (mHasLocals)
            mLocals.write (objectState.mLocals, scriptId);

        objectState.mEnabled = mEnabled;
        objectState.mCount = mCount;
        objectState.mPosition = mPosition;

        for (int i=0; i<3; ++i)
            objectState.mLocalRotation[i] = mLocalRotation.rot[i];
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

    const std::string &RefData::getHandle()
    {
        if(!mBaseNode)
        {
            static const std::string empty;
            return empty;
        }

        return mBaseNode->getName();
    }

    Ogre::SceneNode* RefData::getBaseNode()
    {
        return mBaseNode;
    }

    void RefData::setBaseNode(Ogre::SceneNode* base)
    {
         mBaseNode = base;
    }

    int RefData::getCount() const
    {
        return mCount;
    }

    void RefData::setLocals (const ESM::Script& script)
    {
        if (!mHasLocals)
        {
            mLocals.configure (script);
            mHasLocals = true;
        }
    }

    void RefData::setCount (int count)
    {
        if(count == 0)
            MWBase::Environment::get().getWorld()->removeRefScript(this);

        mCount = count;
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
        mEnabled = true;
    }

    void RefData::disable()
    {
        mEnabled = false;
    }

    ESM::Position& RefData::getPosition()
    {
        return mPosition;
    }

    LocalRotation& RefData::getLocalRotation()
    {
        return mLocalRotation;
    }

    void RefData::setCustomData (CustomData *data)
    {
        delete mCustomData;
        mCustomData = data;
    }

    CustomData *RefData::getCustomData()
    {
        return mCustomData;
    }
}
