#ifndef OPENMW_ESM_OBJECTSTATE_H
#define OPENMW_ESM_OBJECTSTATE_H

#include <string>
#include <vector>

#include <components/esm/luascripts.hpp>
#include <components/esm/position.hpp>
#include <components/esm3/formatversion.hpp>

#include "animationstate.hpp"
#include "cellref.hpp"
#include "locals.hpp"

namespace ESM
{
    class ActorIdConverter;
    class ESMReader;
    class ESMWriter;
    struct ContainerState;
    struct CreatureLevListState;
    struct CreatureState;
    struct DoorState;
    struct NpcState;

    // format 0, saved games only

    ///< \brief Save state for objects, that do not use custom data
    struct ObjectState
    {
        CellRef mRef;

        Locals mLocals;
        LuaScripts mLuaScripts;
        Position mPosition;
        AnimationState mAnimationState;
        ActorIdConverter* mActorIdConverter = nullptr;
        FormatVersion mVersion = DefaultFormatVersion;
        uint32_t mFlags = 0;
        unsigned char mHasLocals = 0;
        unsigned char mEnabled = 0;

        // Is there any class-specific state following the ObjectState
        bool mHasCustomState = true;

        /// @note Does not load the CellRef ID, it should already be loaded before calling this method
        virtual void load(ESMReader& esm);

        virtual void save(ESMWriter& esm, bool inInventory = false) const;

        /// Initialize to default state
        virtual void blank();

        virtual ~ObjectState();

        virtual const NpcState& asNpcState() const;
        virtual NpcState& asNpcState();

        virtual const CreatureState& asCreatureState() const;
        virtual CreatureState& asCreatureState();

        virtual const ContainerState& asContainerState() const;
        virtual ContainerState& asContainerState();

        virtual const DoorState& asDoorState() const;
        virtual DoorState& asDoorState();

        virtual const CreatureLevListState& asCreatureLevListState() const;
        virtual CreatureLevListState& asCreatureLevListState();
    };
}

#endif
