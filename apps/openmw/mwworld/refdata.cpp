
#include "refdata.hpp"

#include "customdata.hpp"

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

        mCreatureStats = refData.mCreatureStats;
        mMovement = refData.mMovement;
        mContainerStore = refData.mContainerStore;

        mCustomData = refData.mCustomData ? refData.mCustomData->clone() : 0;
    }

    void RefData::cleanup()
    {
        mBaseNode = 0;

        delete mCustomData;
        mCustomData = 0;
    }

    RefData::RefData (const ESMS::CellRef& cellRef)
    : mBaseNode(0), mHasLocals (false), mEnabled (true), mCount (1), mPosition (cellRef.pos),
      mCustomData (0)
    {}

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

    RefData::RefData& RefData::operator= (const RefData& refData)
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

    std::string RefData::getHandle()
    {
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
        mEnabled = true;
    }

    boost::shared_ptr<MWMechanics::CreatureStats>& RefData::getCreatureStats()
    {
        return mCreatureStats;
    }

    boost::shared_ptr<MWMechanics::Movement>& RefData::getMovement()
    {
        return mMovement;
    }

    boost::shared_ptr<ContainerStore<RefData> >& RefData::getContainerStore()
    {
        return mContainerStore;
    }

    ESM::Position& RefData::getPosition()
    {
        return mPosition;
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
