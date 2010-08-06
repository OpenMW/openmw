
#include "apparatus.hpp"

#include <components/esm/loadappa.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

namespace MWClass
{
    std::string Apparatus::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->base->name;
    }

    std::string Apparatus::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->base->script;
    }

    void Apparatus::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Apparatus);

        registerClass (typeid (ESM::Apparatus).name(), instance);
    }
}
