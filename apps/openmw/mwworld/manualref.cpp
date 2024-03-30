#include "manualref.hpp"
#include <components/esm/records.hpp>
#include <components/esm4/loadstat.hpp>

#include "esmstore.hpp"

namespace
{

    template <typename T>
    void create(const MWWorld::Store<T>& list, const ESM::RefId& name, std::any& refValue, MWWorld::Ptr& ptrValue)
    {
        const T* base = list.find(name);

        ESM::CellRef cellRef;
        cellRef.blank();
        cellRef.mRefID = name;

        refValue = MWWorld::LiveCellRef<T>(cellRef, base);
        ptrValue = MWWorld::Ptr(&std::any_cast<MWWorld::LiveCellRef<T>&>(refValue), nullptr);
    }
}

MWWorld::ManualRef::ManualRef(const MWWorld::ESMStore& store, const ESM::RefId& name, const int count)
{
    switch (store.find(name))
    {
        case ESM::REC_ACTI:
            create(store.get<ESM::Activator>(), name, mRef, mPtr);
            break;
        case ESM::REC_ALCH:
            create(store.get<ESM::Potion>(), name, mRef, mPtr);
            break;
        case ESM::REC_APPA:
            create(store.get<ESM::Apparatus>(), name, mRef, mPtr);
            break;
        case ESM::REC_ARMO:
            create(store.get<ESM::Armor>(), name, mRef, mPtr);
            break;
        case ESM::REC_BOOK:
            create(store.get<ESM::Book>(), name, mRef, mPtr);
            break;
        case ESM::REC_CLOT:
            create(store.get<ESM::Clothing>(), name, mRef, mPtr);
            break;
        case ESM::REC_CONT:
            create(store.get<ESM::Container>(), name, mRef, mPtr);
            break;
        case ESM::REC_CREA:
            create(store.get<ESM::Creature>(), name, mRef, mPtr);
            break;
        case ESM::REC_DOOR:
            create(store.get<ESM::Door>(), name, mRef, mPtr);
            break;
        case ESM::REC_INGR:
            create(store.get<ESM::Ingredient>(), name, mRef, mPtr);
            break;
        case ESM::REC_LEVC:
            create(store.get<ESM::CreatureLevList>(), name, mRef, mPtr);
            break;
        case ESM::REC_LEVI:
            create(store.get<ESM::ItemLevList>(), name, mRef, mPtr);
            break;
        case ESM::REC_LIGH:
            create(store.get<ESM::Light>(), name, mRef, mPtr);
            break;
        case ESM::REC_LOCK:
            create(store.get<ESM::Lockpick>(), name, mRef, mPtr);
            break;
        case ESM::REC_MISC:
            create(store.get<ESM::Miscellaneous>(), name, mRef, mPtr);
            break;
        case ESM::REC_NPC_:
            create(store.get<ESM::NPC>(), name, mRef, mPtr);
            break;
        case ESM::REC_PROB:
            create(store.get<ESM::Probe>(), name, mRef, mPtr);
            break;
        case ESM::REC_REPA:
            create(store.get<ESM::Repair>(), name, mRef, mPtr);
            break;
        case ESM::REC_STAT:
            create(store.get<ESM::Static>(), name, mRef, mPtr);
            break;
        case ESM::REC_WEAP:
            create(store.get<ESM::Weapon>(), name, mRef, mPtr);
            break;
        case ESM::REC_BODY:
            create(store.get<ESM::BodyPart>(), name, mRef, mPtr);
            break;
        case ESM::REC_STAT4:
            create(store.get<ESM4::Static>(), name, mRef, mPtr);
            break;
        case ESM::REC_TERM4:
            create(store.get<ESM4::Terminal>(), name, mRef, mPtr);
            break;
        case 0:
            throw std::logic_error("failed to create manual cell ref for " + name.toDebugString() + " (unknown ID)");

        default:
            throw std::logic_error("failed to create manual cell ref for " + name.toDebugString() + " (unknown type)");
    }

    mPtr.getCellRef().setCount(count);
}
