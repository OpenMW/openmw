
#include "misc.hpp"

#include <components/esm/loadmisc.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

namespace MWClass
{
    std::string Misc::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Misc, MWWorld::RefData> *ref =
            ptr.get<ESM::Misc>();

        return ref->base->name;
    }

    std::string Misc::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Misc, MWWorld::RefData> *ref =
            ptr.get<ESM::Misc>();

        return ref->base->script;
    }

    void Misc::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Misc);

        registerClass (typeid (ESM::Misc).name(), instance);
    }
}
