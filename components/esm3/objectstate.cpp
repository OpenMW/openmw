#include "objectstate.hpp"

#include <array>
#include <sstream>
#include <stdexcept>
#include <typeinfo>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void ObjectState::load(ESMReader& esm)
    {
        mVersion = esm.getFormatVersion();
        mActorIdConverter = esm.mActorIdConverter;

        bool isDeleted;
        mRef.loadData(esm, isDeleted);

        mHasLocals = 0;
        esm.getHNOT(mHasLocals, "HLOC");

        if (mHasLocals)
            mLocals.load(esm);

        mLuaScripts.load(esm);

        mEnabled = 1;
        esm.getHNOT(mEnabled, "ENAB");

        if (mVersion <= MaxOldCountFormatVersion)
        {
            if (mVersion <= MaxOldGoldValueFormatVersion)
                mRef.mCount = std::max(1, mRef.mCount);
            esm.getHNOT("COUN", mRef.mCount);
        }

        mPosition = mRef.mPos;
        esm.getOptionalComposite("POS_", mPosition);

        mFlags = 0;
        esm.getHNOT(mFlags, "FLAG");

        mAnimationState.load(esm);

        // FIXME: assuming "false" as default would make more sense, but also break compatibility with older save files
        mHasCustomState = true;
        esm.getHNOT(mHasCustomState, "HCUS");
    }

    void ObjectState::save(ESMWriter& esm, bool inInventory) const
    {
        mRef.save(esm, true, inInventory);

        if (mHasLocals)
        {
            esm.writeHNT("HLOC", mHasLocals);
            mLocals.save(esm);
        }

        mLuaScripts.save(esm);

        if (!mEnabled && !inInventory)
            esm.writeHNT("ENAB", mEnabled);

        if (!inInventory && mPosition != mRef.mPos)
        {
            esm.writeNamedComposite("POS_", mPosition);
        }

        if (mFlags != 0)
            esm.writeHNT("FLAG", mFlags);

        mAnimationState.save(esm);

        if (!mHasCustomState)
            esm.writeHNT("HCUS", false);
    }

    void ObjectState::blank()
    {
        mRef.blank();
        mHasLocals = 0;
        mEnabled = false;
        for (int i = 0; i < 3; ++i)
        {
            mPosition.pos[i] = 0;
            mPosition.rot[i] = 0;
        }
        mFlags = 0;
        mHasCustomState = true;
    }

    const NpcState& ObjectState::asNpcState() const
    {
        std::stringstream error;
        error << "bad cast " << typeid(this).name() << " to NpcState";
        throw std::logic_error(error.str());
    }

    NpcState& ObjectState::asNpcState()
    {
        std::stringstream error;
        error << "bad cast " << typeid(this).name() << " to NpcState";
        throw std::logic_error(error.str());
    }

    const CreatureState& ObjectState::asCreatureState() const
    {
        std::stringstream error;
        error << "bad cast " << typeid(this).name() << " to CreatureState";
        throw std::logic_error(error.str());
    }

    CreatureState& ObjectState::asCreatureState()
    {
        std::stringstream error;
        error << "bad cast " << typeid(this).name() << " to CreatureState";
        throw std::logic_error(error.str());
    }

    const ContainerState& ObjectState::asContainerState() const
    {
        std::stringstream error;
        error << "bad cast " << typeid(this).name() << " to ContainerState";
        throw std::logic_error(error.str());
    }

    ContainerState& ObjectState::asContainerState()
    {
        std::stringstream error;
        error << "bad cast " << typeid(this).name() << " to ContainerState";
        throw std::logic_error(error.str());
    }

    const DoorState& ObjectState::asDoorState() const
    {
        std::stringstream error;
        error << "bad cast " << typeid(this).name() << " to DoorState";
        throw std::logic_error(error.str());
    }

    DoorState& ObjectState::asDoorState()
    {
        std::stringstream error;
        error << "bad cast " << typeid(this).name() << " to DoorState";
        throw std::logic_error(error.str());
    }

    const CreatureLevListState& ObjectState::asCreatureLevListState() const
    {
        std::stringstream error;
        error << "bad cast " << typeid(this).name() << " to CreatureLevListState";
        throw std::logic_error(error.str());
    }

    CreatureLevListState& ObjectState::asCreatureLevListState()
    {
        std::stringstream error;
        error << "bad cast " << typeid(this).name() << " to CreatureLevListState";
        throw std::logic_error(error.str());
    }

    ObjectState::~ObjectState() = default;

}
