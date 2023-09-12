#include "itemstats.hpp"

namespace MWLua
{
    ItemStat::ItemStat(const sol::object& object)
        : mObject(ObjectVariant(object))
    {
    }
    sol::optional<double> ItemStat::getCondition() const
    {
        MWWorld::Ptr o = mObject.ptr();
        if (o.getClass().isLight(o))
            return o.getClass().getRemainingUsageTime(o);
        else if (o.getClass().hasItemHealth(o))
            return o.getClass().getItemHealth(o);
        else
            return sol::nullopt;
    }
    void ItemStat::setCondition(float cond) const
    {
        if (!mObject.isGObject())
            throw std::runtime_error("This property can only be set in global scripts");

        MWWorld::Ptr o = mObject.ptr();
        if (o.getClass().isLight(o))
            return o.getClass().setRemainingUsageTime(o, cond);
        else if (o.getClass().hasItemHealth(o))
            o.getCellRef().setCharge(std::max(0, static_cast<int>(cond)));
        else
            throw std::runtime_error("'condition' property does not exist for " + std::string(o.getClass().getName(o))
                + "(" + std::string(o.getTypeDescription()) + ")");
    };
}
