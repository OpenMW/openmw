#include "manualref.hpp"

#include "esmstore.hpp"

namespace
{

    template<typename T>
    void create(const MWWorld::Store<T>& list, const std::string& name, boost::any& refValue, MWWorld::Ptr& ptrValue)
    {
        const T* base = list.find(name);

        ESM::CellRef cellRef;
        cellRef.mRefNum.unset();
        cellRef.mRefID = name;
        cellRef.mScale = 1;
        cellRef.mFactionRank = 0;
        cellRef.mChargeInt = -1;
        cellRef.mChargeIntRemainder = 0.0f;
        cellRef.mGoldValue = 1;
        cellRef.mEnchantmentCharge = -1;
        cellRef.mTeleport = false;
        cellRef.mLockLevel = 0;
        cellRef.mReferenceBlocked = 0;

        MWWorld::LiveCellRef<T> ref(cellRef, base);

        refValue = ref;
        ptrValue = MWWorld::Ptr(&boost::any_cast<MWWorld::LiveCellRef<T>&>(refValue), nullptr);
    }
}

MWWorld::ManualRef::ManualRef(const MWWorld::ESMStore& store, const std::string& name, const int count)
{
    std::string lowerName = Misc::StringUtils::lowerCase(name);
    switch (store.find(lowerName))
    {
    case ESM::REC_ACTI: create(store.get<ESM::Activator>(), lowerName, mRef, mPtr); break;
    case ESM::REC_ALCH: create(store.get<ESM::Potion>(), lowerName, mRef, mPtr); break;
    case ESM::REC_APPA: create(store.get<ESM::Apparatus>(), lowerName, mRef, mPtr); break;
    case ESM::REC_ARMO: create(store.get<ESM::Armor>(), lowerName, mRef, mPtr); break;
    case ESM::REC_BOOK: create(store.get<ESM::Book>(), lowerName, mRef, mPtr); break;
    case ESM::REC_CLOT: create(store.get<ESM::Clothing>(), lowerName, mRef, mPtr); break;
    case ESM::REC_CONT: create(store.get<ESM::Container>(), lowerName, mRef, mPtr); break;
    case ESM::REC_CREA: create(store.get<ESM::Creature>(), lowerName, mRef, mPtr); break;
    case ESM::REC_DOOR: create(store.get<ESM::Door>(), lowerName, mRef, mPtr); break;
    case ESM::REC_INGR: create(store.get<ESM::Ingredient>(), lowerName, mRef, mPtr); break;
    case ESM::REC_LEVC: create(store.get<ESM::CreatureLevList>(), lowerName, mRef, mPtr); break;
    case ESM::REC_LEVI: create(store.get<ESM::ItemLevList>(), lowerName, mRef, mPtr); break;
    case ESM::REC_LIGH: create(store.get<ESM::Light>(), lowerName, mRef, mPtr); break;
    case ESM::REC_LOCK: create(store.get<ESM::Lockpick>(), lowerName, mRef, mPtr); break;
    case ESM::REC_MISC: create(store.get<ESM::Miscellaneous>(), lowerName, mRef, mPtr); break;
    case ESM::REC_NPC_: create(store.get<ESM::NPC>(), lowerName, mRef, mPtr); break;
    case ESM::REC_PROB: create(store.get<ESM::Probe>(), lowerName, mRef, mPtr); break;
    case ESM::REC_REPA: create(store.get<ESM::Repair>(), lowerName, mRef, mPtr); break;
    case ESM::REC_STAT: create(store.get<ESM::Static>(), lowerName, mRef, mPtr); break;
    case ESM::REC_WEAP: create(store.get<ESM::Weapon>(), lowerName, mRef, mPtr); break;
    case ESM::REC_BODY: create(store.get<ESM::BodyPart>(), lowerName, mRef, mPtr); break;

    case 0:
        throw std::logic_error("failed to create manual cell ref for " + lowerName + " (unknown ID)");

    default:
        throw std::logic_error("failed to create manual cell ref for " + lowerName + " (unknown type)");
    }

    mPtr.getRefData().setCount(count);
}
