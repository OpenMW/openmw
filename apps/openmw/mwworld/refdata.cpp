
#include "refdata.hpp"

namespace MWWorld
{
    RefData::RefData (const ESMS::CellRef& cellRef)
    : mBaseNode(0), mHasLocals (false), mEnabled (true), mCount (1), mPosition (cellRef.pos)
    {}

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

    boost::shared_ptr<MWMechanics::NpcStats>& RefData::getNpcStats()
    {
        return mNpcStats;
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
}
