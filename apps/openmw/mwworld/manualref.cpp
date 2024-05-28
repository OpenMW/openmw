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

    template <typename T>
    void create(
        const MWWorld::Store<T>& list, const MWWorld::Ptr& template_, std::any& refValue, MWWorld::Ptr& ptrValue)
    {
        refValue = *static_cast<MWWorld::LiveCellRef<T>*>(template_.getBase());
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

MWWorld::ManualRef::ManualRef(const ESMStore& store, const Ptr& template_, const int count)
{
    ESM::RefId name = template_.getCellRef().getRefId();
    switch (store.find(name))
    {
        case ESM::REC_ACTI:
            create(store.get<ESM::Activator>(), template_, mRef, mPtr);
            break;
        case ESM::REC_ALCH:
            create(store.get<ESM::Potion>(), template_, mRef, mPtr);
            break;
        case ESM::REC_APPA:
            create(store.get<ESM::Apparatus>(), template_, mRef, mPtr);
            break;
        case ESM::REC_ARMO:
            create(store.get<ESM::Armor>(), template_, mRef, mPtr);
            break;
        case ESM::REC_BOOK:
            create(store.get<ESM::Book>(), template_, mRef, mPtr);
            break;
        case ESM::REC_CLOT:
            create(store.get<ESM::Clothing>(), template_, mRef, mPtr);
            break;
        case ESM::REC_CONT:
            create(store.get<ESM::Container>(), template_, mRef, mPtr);
            break;
        case ESM::REC_CREA:
            create(store.get<ESM::Creature>(), template_, mRef, mPtr);
            break;
        case ESM::REC_DOOR:
            create(store.get<ESM::Door>(), template_, mRef, mPtr);
            break;
        case ESM::REC_INGR:
            create(store.get<ESM::Ingredient>(), template_, mRef, mPtr);
            break;
        case ESM::REC_LEVC:
            create(store.get<ESM::CreatureLevList>(), template_, mRef, mPtr);
            break;
        case ESM::REC_LEVI:
            create(store.get<ESM::ItemLevList>(), template_, mRef, mPtr);
            break;
        case ESM::REC_LIGH:
            create(store.get<ESM::Light>(), template_, mRef, mPtr);
            break;
        case ESM::REC_LOCK:
            create(store.get<ESM::Lockpick>(), template_, mRef, mPtr);
            break;
        case ESM::REC_MISC:
            create(store.get<ESM::Miscellaneous>(), template_, mRef, mPtr);
            break;
        case ESM::REC_NPC_:
            create(store.get<ESM::NPC>(), template_, mRef, mPtr);
            break;
        case ESM::REC_PROB:
            create(store.get<ESM::Probe>(), template_, mRef, mPtr);
            break;
        case ESM::REC_REPA:
            create(store.get<ESM::Repair>(), template_, mRef, mPtr);
            break;
        case ESM::REC_STAT:
            create(store.get<ESM::Static>(), template_, mRef, mPtr);
            break;
        case ESM::REC_WEAP:
            create(store.get<ESM::Weapon>(), template_, mRef, mPtr);
            break;
        case ESM::REC_BODY:
            create(store.get<ESM::BodyPart>(), template_, mRef, mPtr);
            break;
        case ESM::REC_STAT4:
            create(store.get<ESM4::Static>(), template_, mRef, mPtr);
            break;
        case ESM::REC_TERM4:
            create(store.get<ESM4::Terminal>(), template_, mRef, mPtr);
            break;
        case 0:
            throw std::logic_error("failed to create manual cell ref for " + name.toDebugString() + " (unknown ID)");

        default:
            throw std::logic_error("failed to create manual cell ref for " + name.toDebugString() + " (unknown type)");
    }

    mPtr.getCellRef().setCount(count);
    mPtr.getCellRef().unsetRefNum();
    mPtr.getRefData().setLuaScripts(nullptr);
}
