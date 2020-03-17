#include "creaturelevlist.hpp"

#include <components/esm/loadlevlist.hpp>
#include <components/esm/creaturelevliststate.hpp>

#include "../mwmechanics/levelledlist.hpp"

#include "../mwworld/customdata.hpp"
#include "../mwmechanics/creaturestats.hpp"

namespace MWClass
{
    class CreatureLevListCustomData : public MWWorld::CustomData
    {
    public:
        // actorId of the creature we spawned
        int mSpawnActorId;
        bool mSpawn; // Should a new creature be spawned?

        virtual MWWorld::CustomData *clone() const;

        virtual CreatureLevListCustomData& asCreatureLevListCustomData()
        {
            return *this;
        }
        virtual const CreatureLevListCustomData& asCreatureLevListCustomData() const
        {
            return *this;
        }
    };

    MWWorld::CustomData *CreatureLevListCustomData::clone() const
    {
        return new CreatureLevListCustomData (*this);
    }

    std::string CreatureLevList::getName (const MWWorld::ConstPtr& ptr) const
    {
        return "";
    }

    bool CreatureLevList::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        return false;
    }

    void CreatureLevList::respawn(const MWWorld::Ptr &ptr) const
    {
        ensureCustomData(ptr);

        CreatureLevListCustomData& customData = ptr.getRefData().getCustomData()->asCreatureLevListCustomData();
        if (customData.mSpawn)
            return;

        MWWorld::Ptr creature = (customData.mSpawnActorId == -1) ? MWWorld::Ptr() : MWBase::Environment::get().getWorld()->searchPtrViaActorId(customData.mSpawnActorId);
        if (!creature.isEmpty())
        {
            const MWMechanics::CreatureStats& creatureStats = creature.getClass().getCreatureStats(creature);
            if (creature.getRefData().getCount() == 0)
                customData.mSpawn = true;
            else if (creatureStats.isDead())
            {
                const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
                static const float fCorpseRespawnDelay = gmst.find("fCorpseRespawnDelay")->mValue.getFloat();
                static const float fCorpseClearDelay = gmst.find("fCorpseClearDelay")->mValue.getFloat();

                float delay = std::min(fCorpseRespawnDelay, fCorpseClearDelay);
                if (creatureStats.getTimeOfDeath() + delay <= MWBase::Environment::get().getWorld()->getTimeStamp())
                    customData.mSpawn = true;
            }
        }
        else
            customData.mSpawn = true;
    }

    void CreatureLevList::registerSelf()
    {
        std::shared_ptr<Class> instance (new CreatureLevList);

        registerClass (typeid (ESM::CreatureLevList).name(), instance);
    }

    void CreatureLevList::getModelsToPreload(const MWWorld::Ptr &ptr, std::vector<std::string> &models) const
    {
        // disable for now, too many false positives
        /*
        const MWWorld::LiveCellRef<ESM::CreatureLevList> *ref = ptr.get<ESM::CreatureLevList>();
        for (std::vector<ESM::LevelledListBase::LevelItem>::const_iterator it = ref->mBase->mList.begin(); it != ref->mBase->mList.end(); ++it)
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            if (it->mLevel > player.getClass().getCreatureStats(player).getLevel())
                continue;

            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            MWWorld::ManualRef ref(store, it->mId);
            ref.getPtr().getClass().getModelsToPreload(ref.getPtr(), models);
        }
        */
    }

    void CreatureLevList::insertObjectRendering(const MWWorld::Ptr &ptr, const std::string& model, MWRender::RenderingInterface &renderingInterface) const
    {
        ensureCustomData(ptr);

        CreatureLevListCustomData& customData = ptr.getRefData().getCustomData()->asCreatureLevListCustomData();
        if (!customData.mSpawn)
            return;

        MWWorld::LiveCellRef<ESM::CreatureLevList> *ref =
            ptr.get<ESM::CreatureLevList>();

        std::string id = MWMechanics::getLevelledItem(ref->mBase, true);

        if (!id.empty())
        {
            // Delete the previous creature
            if (customData.mSpawnActorId != -1)
            {
                MWWorld::Ptr creature = MWBase::Environment::get().getWorld()->searchPtrViaActorId(customData.mSpawnActorId);
                if (!creature.isEmpty())
                    MWBase::Environment::get().getWorld()->deleteObject(creature);
                customData.mSpawnActorId = -1;
            }

            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            MWWorld::ManualRef manualRef(store, id);
            manualRef.getPtr().getCellRef().setPosition(ptr.getCellRef().getPosition());
            MWWorld::Ptr placed = MWBase::Environment::get().getWorld()->placeObject(manualRef.getPtr(), ptr.getCell() , ptr.getCellRef().getPosition());
            customData.mSpawnActorId = placed.getClass().getCreatureStats(placed).getActorId();
            customData.mSpawn = false;
        }
        else
            customData.mSpawn = false;
    }

    void CreatureLevList::ensureCustomData(const MWWorld::Ptr &ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::unique_ptr<CreatureLevListCustomData> data (new CreatureLevListCustomData);
            data->mSpawnActorId = -1;
            data->mSpawn = true;

            ptr.getRefData().setCustomData(data.release());
        }
    }

    void CreatureLevList::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
        const
    {
        if (!state.mHasCustomState)
            return;

        ensureCustomData(ptr);
        CreatureLevListCustomData& customData = ptr.getRefData().getCustomData()->asCreatureLevListCustomData();
        const ESM::CreatureLevListState& levListState = state.asCreatureLevListState();
        customData.mSpawnActorId = levListState.mSpawnActorId;
        customData.mSpawn = levListState.mSpawn;
    }

    void CreatureLevList::writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state)
        const
    {
        if (!ptr.getRefData().getCustomData())
        {
            state.mHasCustomState = false;
            return;
        }

        const CreatureLevListCustomData& customData = ptr.getRefData().getCustomData()->asCreatureLevListCustomData();
        ESM::CreatureLevListState& levListState = state.asCreatureLevListState();
        levListState.mSpawnActorId = customData.mSpawnActorId;
        levListState.mSpawn = customData.mSpawn;
    }
}
