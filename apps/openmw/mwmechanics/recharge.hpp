#ifndef MWMECHANICS_RECHARGE_H
#define MWMECHANICS_RECHARGE_H

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{

    bool rechargeItem(const MWWorld::Ptr &item, const float maxCharge, const float duration);

    bool rechargeItem(const MWWorld::Ptr &item, const MWWorld::Ptr &gem);

}

#endif
