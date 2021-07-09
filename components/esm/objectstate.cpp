#include "objectstate.hpp"

#include <stdexcept>
#include <sstream>
#include <typeinfo>

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::ObjectState::load (ESMReader &esm)
{
    mVersion = esm.getFormat();

    bool isDeleted;
    mRef.loadData(esm, isDeleted);

    mHasLocals = 0;
    esm.getHNOT (mHasLocals, "HLOC");

    if (mHasLocals)
        mLocals.load (esm);

    mLuaScripts.load(esm);

    mEnabled = 1;
    esm.getHNOT (mEnabled, "ENAB");

    mCount = 1;
    esm.getHNOT (mCount, "COUN");

    mPosition = mRef.mPos;
    esm.getHNOT (mPosition, "POS_", 24);

    if (esm.isNextSub("LROT"))
        esm.skipHSub(); // local rotation, no longer used

    mFlags = 0;
    esm.getHNOT (mFlags, "FLAG");

    // obsolete
    int unused;
    esm.getHNOT(unused, "LTIM");

    mAnimationState.load(esm);

    // FIXME: assuming "false" as default would make more sense, but also break compatibility with older save files
    mHasCustomState = true;
    esm.getHNOT (mHasCustomState, "HCUS");
}

void ESM::ObjectState::save (ESMWriter &esm, bool inInventory) const
{
    mRef.save (esm, true, inInventory);

    if (mHasLocals)
    {
        esm.writeHNT ("HLOC", mHasLocals);
        mLocals.save (esm);
    }

    mLuaScripts.save(esm);

    if (!mEnabled && !inInventory)
        esm.writeHNT ("ENAB", mEnabled);

    if (mCount!=1)
        esm.writeHNT ("COUN", mCount);

    if (!inInventory && mPosition != mRef.mPos)
        esm.writeHNT ("POS_", mPosition, 24);

    if (mFlags != 0)
        esm.writeHNT ("FLAG", mFlags);

    mAnimationState.save(esm);

    if (!mHasCustomState)
        esm.writeHNT ("HCUS", false);
}

void ESM::ObjectState::blank()
{
    mRef.blank();
    mHasLocals = 0;
    mEnabled = false;
    mCount = 1;
    for (int i=0;i<3;++i)
    {
        mPosition.pos[i] = 0;
        mPosition.rot[i] = 0;
    }
    mFlags = 0;
    mHasCustomState = true;
}

const ESM::NpcState& ESM::ObjectState::asNpcState() const
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to NpcState";
    throw std::logic_error(error.str());
}

ESM::NpcState& ESM::ObjectState::asNpcState()
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to NpcState";
    throw std::logic_error(error.str());
}

const ESM::CreatureState& ESM::ObjectState::asCreatureState() const
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to CreatureState";
    throw std::logic_error(error.str());
}

ESM::CreatureState& ESM::ObjectState::asCreatureState()
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to CreatureState";
    throw std::logic_error(error.str());
}

const ESM::ContainerState& ESM::ObjectState::asContainerState() const
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to ContainerState";
    throw std::logic_error(error.str());
}

ESM::ContainerState& ESM::ObjectState::asContainerState()
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to ContainerState";
    throw std::logic_error(error.str());
}

const ESM::DoorState& ESM::ObjectState::asDoorState() const
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to DoorState";
    throw std::logic_error(error.str());
}

ESM::DoorState& ESM::ObjectState::asDoorState()
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to DoorState";
    throw std::logic_error(error.str());
}

const ESM::CreatureLevListState& ESM::ObjectState::asCreatureLevListState() const
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to CreatureLevListState";
    throw std::logic_error(error.str());
}

ESM::CreatureLevListState& ESM::ObjectState::asCreatureLevListState()
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to CreatureLevListState";
    throw std::logic_error(error.str());
}

ESM::ObjectState::~ObjectState() {}
