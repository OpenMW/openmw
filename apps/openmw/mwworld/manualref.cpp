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

    template <typename F>
    void createGeneric(F func, const MWWorld::ESMStore& store, ESM::RefId name)
    {
        switch (store.find(name))
        {
            case ESM::REC_ACTI:
                func(store.get<ESM::Activator>());
                break;
            case ESM::REC_ALCH:
                func(store.get<ESM::Potion>());
                break;
            case ESM::REC_APPA:
                func(store.get<ESM::Apparatus>());
                break;
            case ESM::REC_ARMO:
                func(store.get<ESM::Armor>());
                break;
            case ESM::REC_BOOK:
                func(store.get<ESM::Book>());
                break;
            case ESM::REC_CLOT:
                func(store.get<ESM::Clothing>());
                break;
            case ESM::REC_CONT:
                func(store.get<ESM::Container>());
                break;
            case ESM::REC_CREA:
                func(store.get<ESM::Creature>());
                break;
            case ESM::REC_DOOR:
                func(store.get<ESM::Door>());
                break;
            case ESM::REC_INGR:
                func(store.get<ESM::Ingredient>());
                break;
            case ESM::REC_LEVC:
                func(store.get<ESM::CreatureLevList>());
                break;
            case ESM::REC_LEVI:
                func(store.get<ESM::ItemLevList>());
                break;
            case ESM::REC_LIGH:
                func(store.get<ESM::Light>());
                break;
            case ESM::REC_LOCK:
                func(store.get<ESM::Lockpick>());
                break;
            case ESM::REC_MISC:
                func(store.get<ESM::Miscellaneous>());
                break;
            case ESM::REC_NPC_:
                func(store.get<ESM::NPC>());
                break;
            case ESM::REC_PROB:
                func(store.get<ESM::Probe>());
                break;
            case ESM::REC_REPA:
                func(store.get<ESM::Repair>());
                break;
            case ESM::REC_STAT:
                func(store.get<ESM::Static>());
                break;
            case ESM::REC_WEAP:
                func(store.get<ESM::Weapon>());
                break;
            case ESM::REC_BODY:
                func(store.get<ESM::BodyPart>());
                break;
            case ESM::REC_STAT4:
                func(store.get<ESM4::Static>());
                break;
            case ESM::REC_TERM4:
                func(store.get<ESM4::Terminal>());
                break;
            case 0:
                throw std::logic_error(
                    "failed to create manual cell ref for " + name.toDebugString() + " (unknown ID)");

            default:
                throw std::logic_error(
                    "failed to create manual cell ref for " + name.toDebugString() + " (unknown type)");
        }
    }
}

MWWorld::ManualRef::ManualRef(const MWWorld::ESMStore& store, const ESM::RefId& name, const int count)
{
    auto cb = [&](const auto& store) { create(store, name, mRef, mPtr); };
    createGeneric(cb, store, name);

    mPtr.getCellRef().setCount(count);
}

MWWorld::ManualRef::ManualRef(const ESMStore& store, const Ptr& template_, const int count)
{
    auto cb = [&](const auto& store) { create(store, template_, mRef, mPtr); };
    createGeneric(cb, store, template_.getCellRef().getRefId());

    mPtr.getCellRef().setCount(count);
    mPtr.getCellRef().unsetRefNum();
    mPtr.getRefData().setLuaScripts(nullptr);
}
