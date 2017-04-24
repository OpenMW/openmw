#ifndef OPENMW_LOCALACTOR_HPP
#define OPENMW_LOCALACTOR_HPP

#include <components/openmw-mp/Base/BaseActor.hpp>
#include "../mwmechanics/creaturestats.hpp"
#include "../mwworld/manualref.hpp"

namespace mwmp
{
    class LocalActor : public BaseActor
    {
    public:

        LocalActor();
        virtual ~LocalActor();

        void update(bool forceUpdate);

        void updateCell();
        void updatePosition(bool forceUpdate);
        void updateAnimFlags(bool forceUpdate);
        void updateAnimPlay();
        void updateSpeech();
        void updateStatsDynamic(bool forceUpdate);
        void updateAttack();

        MWWorld::Ptr getPtr();
        void setPtr(const MWWorld::Ptr& newPtr);

    private:
        MWWorld::Ptr ptr;

        bool posWasChanged;

        bool wasRunning;
        bool wasSneaking;
        bool wasForceJumping;
        bool wasForceMoveJumping;

        bool wasJumping;
        bool wasFlying;

        MWMechanics::DrawState_ lastDrawState;

        MWMechanics::DynamicStat<float> oldHealth;
        MWMechanics::DynamicStat<float> oldMagicka;
        MWMechanics::DynamicStat<float> oldFatigue;
        float statTimer;
    };
}

#endif //OPENMW_LOCALACTOR_HPP
