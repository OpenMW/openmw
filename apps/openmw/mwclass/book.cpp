
#include "book.hpp"

#include <components/esm/loadbook.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"

#include "containerutil.hpp"

namespace MWClass
{
    std::string Book::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Book, MWWorld::RefData> *ref =
            ptr.get<ESM::Book>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Book::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        // TODO implement reading

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    void Book::insertIntoContainer (const MWWorld::Ptr& ptr,
        MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const
    {
        insertIntoContainerStore (ptr, containerStore.books);
    }

    std::string Book::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Book, MWWorld::RefData> *ref =
            ptr.get<ESM::Book>();

        return ref->base->script;
    }

    void Book::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Book);

        registerClass (typeid (ESM::Book).name(), instance);
    }
}
