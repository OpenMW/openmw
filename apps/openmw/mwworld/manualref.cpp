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
    void visitRefStore(const MWWorld::ESMStore& store, ESM::RefId name, F func)
    {
        switch (store.find(name))
        {
            case ESM::REC_ACTI:
                return func(store.get<ESM::Activator>());
            case ESM::REC_ALCH:
                return func(store.get<ESM::Potion>());
            case ESM::REC_APPA:
                return func(store.get<ESM::Apparatus>());
            case ESM::REC_ARMO:
                return func(store.get<ESM::Armor>());
            case ESM::REC_BOOK:
                return func(store.get<ESM::Book>());
            case ESM::REC_CLOT:
                return func(store.get<ESM::Clothing>());
            case ESM::REC_CONT:
                return func(store.get<ESM::Container>());
            case ESM::REC_CREA:
                return func(store.get<ESM::Creature>());
            case ESM::REC_DOOR:
                return func(store.get<ESM::Door>());
            case ESM::REC_INGR:
                return func(store.get<ESM::Ingredient>());
            case ESM::REC_LEVC:
                return func(store.get<ESM::CreatureLevList>());
            case ESM::REC_LEVI:
                return func(store.get<ESM::ItemLevList>());
            case ESM::REC_LIGH:
                return func(store.get<ESM::Light>());
            case ESM::REC_LOCK:
                return func(store.get<ESM::Lockpick>());
            case ESM::REC_MISC:
                return func(store.get<ESM::Miscellaneous>());
            case ESM::REC_NPC_:
                return func(store.get<ESM::NPC>());
            case ESM::REC_PROB:
                return func(store.get<ESM::Probe>());
            case ESM::REC_REPA:
                return func(store.get<ESM::Repair>());
            case ESM::REC_STAT:
                return func(store.get<ESM::Static>());
            case ESM::REC_WEAP:
                return func(store.get<ESM::Weapon>());
            case ESM::REC_BODY:
                return func(store.get<ESM::BodyPart>());
            case ESM::REC_STAT4:
                return func(store.get<ESM4::Static>());
            case ESM::REC_TERM4:
                return func(store.get<ESM4::Terminal>());
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
    visitRefStore(store, name, cb);

    mPtr.getCellRef().setCount(count);
}

MWWorld::ManualRef::ManualRef(const ESMStore& store, const Ptr& template_, const int count)
{
    auto cb = [&](const auto& store) { create(store, template_, mRef, mPtr); };
    visitRefStore(store, template_.getCellRef().getRefId(), cb);

    mPtr.getCellRef().setCount(count);
    mPtr.getCellRef().unsetRefNum();
    mPtr.getRefData().setLuaScripts(nullptr);
}
