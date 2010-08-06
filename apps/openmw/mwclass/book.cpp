
#include "book.hpp"

#include <components/esm/loadbook.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

#include "containerutil.hpp"

namespace MWClass
{
    std::string Book::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Book, MWWorld::RefData> *ref =
            ptr.get<ESM::Book>();

        return ref->base->name;
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
