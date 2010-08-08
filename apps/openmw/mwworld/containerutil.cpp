
#include "containerutil.hpp"

namespace
{
    template<typename T>
    void listItemsInContainerImp (const std::string& id,
        ESMS::CellRefList<T, MWWorld::RefData>& containerStore,
        const ESMS::RecListT<T>& store, std::vector<MWWorld::Ptr>& list)
    {
        if (const T *record = store.search (id))
        {
            for (typename ESMS::CellRefList<T, MWWorld::RefData>::List::iterator iter
                (containerStore.list.begin());
                iter!=containerStore.list.end(); ++iter)
            {
                if (iter->base==record)
                    list.push_back (MWWorld::Ptr (&*iter, 0));
            }
        }
    }
}

namespace MWWorld
{
    void listItemsInContainer (const std::string& id,
        ContainerStore<MWWorld::RefData>& containerStore,
        const ESMS::ESMStore& store, std::vector<Ptr>& list)
    {
        listItemsInContainerImp (id, containerStore.potions, store.potions, list);
        listItemsInContainerImp (id, containerStore.appas, store.appas, list);
        listItemsInContainerImp (id, containerStore.armors, store.armors, list);
        listItemsInContainerImp (id, containerStore.books, store.books, list);
        listItemsInContainerImp (id, containerStore.clothes, store.clothes, list);
        listItemsInContainerImp (id, containerStore.ingreds, store.ingreds, list);
        listItemsInContainerImp (id, containerStore.lights, store.lights, list);
        listItemsInContainerImp (id, containerStore.lockpicks, store.lockpicks, list);
        listItemsInContainerImp (id, containerStore.miscItems, store.miscItems, list);
        listItemsInContainerImp (id, containerStore.probes, store.probes, list);
        listItemsInContainerImp (id, containerStore.repairs, store.repairs, list);
        listItemsInContainerImp (id, containerStore.weapons, store.weapons, list);
    }
}
