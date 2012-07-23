
#include "cellstore.hpp"

#include <iostream>

#include <components/esm_store/store.hpp>

namespace MWWorld
{
    CellStore::CellStore (const ESM::Cell *cell_) : cell (cell_), mState (State_Unloaded)
    {
        mWaterLevel = cell->water;
    }

    void CellStore::load (const ESMS::ESMStore &store, ESM::ESMReader &esm)
    {
        if (mState!=State_Loaded)
        {
            if (mState==State_Preloaded)
                mIds.clear();

            std::cout << "loading cell " << cell->getDescription() << std::endl;

            loadRefs (store, esm);

            mState = State_Loaded;
        }
    }

    void CellStore::preload (const ESMS::ESMStore &store, ESM::ESMReader &esm)
    {
        if (mState==State_Unloaded)
        {
            listRefs (store, esm);

            mState = State_Preloaded;
        }
    }

    void CellStore::listRefs(const ESMS::ESMStore &store, ESM::ESMReader &esm)
    {
        assert (cell);

        if (cell->context.filename.empty())
            return; // this is a dynamically generated cell -> skipping.

        // Reopen the ESM reader and seek to the right position.
        cell->restore (esm);

        ESM::CellRef ref;

        // Get each reference in turn
        while (cell->getNextRef (esm, ref))
        {
            std::string lowerCase;

            std::transform (ref.refID.begin(), ref.refID.end(), std::back_inserter (lowerCase),
                (int(*)(int)) std::tolower);

            mIds.push_back (lowerCase);
        }

        std::sort (mIds.begin(), mIds.end());
    }

    void CellStore::loadRefs(const ESMS::ESMStore &store, ESM::ESMReader &esm)
    {
      assert (cell);

        if (cell->context.filename.empty())
            return; // this is a dynamically generated cell -> skipping.

      // Reopen the ESM reader and seek to the right position.
      cell->restore(esm);

      ESM::CellRef ref;

      // Get each reference in turn
      while(cell->getNextRef(esm, ref))
        {
            std::string lowerCase;

            std::transform (ref.refID.begin(), ref.refID.end(), std::back_inserter (lowerCase),
                (int(*)(int)) std::tolower);

            int rec = store.find(ref.refID);

            ref.refID = lowerCase;

          /* We can optimize this further by storing the pointer to the
             record itself in store.all, so that we don't need to look it
             up again here. However, never optimize. There are infinite
             opportunities to do that later.
           */
          switch(rec)
            {
            case ESM::REC_ACTI: activators.find(ref, store.activators); break;
            case ESM::REC_ALCH: potions.find(ref, store.potions); break;
            case ESM::REC_APPA: appas.find(ref, store.appas); break;
            case ESM::REC_ARMO: armors.find(ref, store.armors); break;
            case ESM::REC_BOOK: books.find(ref, store.books); break;
            case ESM::REC_CLOT: clothes.find(ref, store.clothes); break;
            case ESM::REC_CONT: containers.find(ref, store.containers); break;
            case ESM::REC_CREA: creatures.find(ref, store.creatures); break;
            case ESM::REC_DOOR: doors.find(ref, store.doors); break;
            case ESM::REC_INGR: ingreds.find(ref, store.ingreds); break;
            case ESM::REC_LEVC: creatureLists.find(ref, store.creatureLists); break;
            case ESM::REC_LEVI: itemLists.find(ref, store.itemLists); break;
            case ESM::REC_LIGH: lights.find(ref, store.lights); break;
            case ESM::REC_LOCK: lockpicks.find(ref, store.lockpicks); break;
            case ESM::REC_MISC: miscItems.find(ref, store.miscItems); break;
            case ESM::REC_NPC_: npcs.find(ref, store.npcs); break;
            case ESM::REC_PROB: probes.find(ref, store.probes); break;
            case ESM::REC_REPA: repairs.find(ref, store.repairs); break;
            case ESM::REC_STAT: statics.find(ref, store.statics); break;
            case ESM::REC_WEAP: weapons.find(ref, store.weapons); break;

            case 0: std::cout << "Cell reference " + ref.refID + " not found!\n"; break;
            default:
              std::cout << "WARNING: Ignoring reference '" << ref.refID << "' of unhandled type\n";
            }
        }
    }

    /// \todo this whole code needs major clean up
    void CellStore::insertObject(const Ptr &ptr)
    {
        std::string type = ptr.getTypeName();

        MWWorld::Ptr newPtr;

        // insert into the correct CellRefList
        if      (type == typeid(ESM::Potion).name())
        {
            MWWorld::LiveCellRef<ESM::Potion>* ref = ptr.get<ESM::Potion>();
            newPtr = MWWorld::Ptr(&potions.insert(*ref), cell);
        }
        else if (type == typeid(ESM::Apparatus).name())
        {
            MWWorld::LiveCellRef<ESM::Apparatus>* ref = ptr.get<ESM::Apparatus>();
            newPtr = MWWorld::Ptr(&appas.insert(*ref), cell);
        }
        else if (type == typeid(ESM::Armor).name())
        {
            MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();
            newPtr = MWWorld::Ptr(&armors.insert(*ref), cell);
        }
        else if (type == typeid(ESM::Book).name())
        {
            MWWorld::LiveCellRef<ESM::Book>* ref = ptr.get<ESM::Book>();
            newPtr = MWWorld::Ptr(&books.insert(*ref), cell);
        }
        else if (type == typeid(ESM::Clothing).name())
        {
            MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();
            newPtr = MWWorld::Ptr(&clothes.insert(*ref), cell);
        }
        else if (type == typeid(ESM::Ingredient).name())
        {
            MWWorld::LiveCellRef<ESM::Ingredient>* ref = ptr.get<ESM::Ingredient>();
            newPtr = MWWorld::Ptr(&ingreds.insert(*ref), cell);
        }
        else if (type == typeid(ESM::Light).name())
        {
            MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();
            newPtr = MWWorld::Ptr(&lights.insert(*ref), cell);
        }
        else if (type == typeid(ESM::Tool).name())
        {
            MWWorld::LiveCellRef<ESM::Tool>* ref = ptr.get<ESM::Tool>();
            newPtr = MWWorld::Ptr(&lockpicks.insert(*ref), cell);
        }
        else if (type == typeid(ESM::Repair).name())
        {
            MWWorld::LiveCellRef<ESM::Repair>* ref = ptr.get<ESM::Repair>();
            newPtr = MWWorld::Ptr(&repairs.insert(*ref), cell);
        }
        else if (type == typeid(ESM::Probe).name())
        {
            MWWorld::LiveCellRef<ESM::Probe>* ref = ptr.get<ESM::Probe>();
            newPtr = MWWorld::Ptr(&probes.insert(*ref), cell);
        }
        else if (type == typeid(ESM::Weapon).name())
        {
            MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();
            newPtr = MWWorld::Ptr(&weapons.insert(*ref), cell);
        }
        else if (type == typeid(ESM::Miscellaneous).name())
        {

            // if this is gold, we need to fetch the correct mesh depending on the amount of gold.
            if (MWWorld::Class::get(ptr).getName(ptr) == MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sGold")->str)
            {
                int goldAmount = ptr.getRefData().getCount();

                std::string base = "Gold_001";
                if (goldAmount >= 100)
                    base = "Gold_100";
                else if (goldAmount >= 25)
                    base = "Gold_025";
                else if (goldAmount >= 10)
                    base = "Gold_010";
                else if (goldAmount >= 5)
                    base = "Gold_005";

                MWWorld::ManualRef newRef (MWBase::Environment::get().getWorld()->getStore(), base);

                MWWorld::LiveCellRef<ESM::Miscellaneous>* ref =
                    newRef.getPtr().get<ESM::Miscellaneous>();

                newPtr = MWWorld::Ptr(&miscItems.insert(*ref), cell);

                ESM::Position& p = newPtr.getRefData().getPosition();
                p.pos[0] = ptr.getRefData().getPosition().pos[0];
                p.pos[1] = ptr.getRefData().getPosition().pos[1];
                p.pos[2] = ptr.getRefData().getPosition().pos[2];
            }
            else
            {
                MWWorld::LiveCellRef<ESM::Miscellaneous>* ref = ptr.get<ESM::Miscellaneous>();
                newPtr = MWWorld::Ptr(&miscItems.insert(*ref), cell);
            }
        }
        else
            throw std::runtime_error("Trying to insert object of unhandled type");

        newPtr.getRefData().setCount(ptr.getRefData().getCount());
        ptr.getRefData().setCount(0);
        newPtr.getRefData().enable();
    }
}
