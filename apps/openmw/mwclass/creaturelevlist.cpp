#include "creaturelevlist.hpp"

#include <components/esm3/creaturelevliststate.hpp>
#include <components/esm3/loadlevlist.hpp>

#include "../mwmechanics/levelledlist.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/manualref.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWClass
{
    class CreatureLevListCustomData : public MWWorld::TypedCustomData<CreatureLevListCustomData>
    {
    public:
        // actorId of the creature we spawned
        int mSpawnActorId;
        bool mSpawn; // Should a new creature be spawned?

        CreatureLevListCustomData& asCreatureLevListCustomData() override { return *this; }
        const CreatureLevListCustomData& asCreatureLevListCustomData() const override { return *this; }
    };

    CreatureLevList::CreatureLevList()
        : MWWorld::RegisteredClass<CreatureLevList>(ESM::CreatureLevList::sRecordId)
    {
    }

    MWWorld::Ptr CreatureLevList::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM::CreatureLevList>* ref = ptr.get<ESM::CreatureLevList>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    void CreatureLevList::adjustPosition(const MWWorld::Ptr& ptr, bool force) const
    {
        if (ptr.getRefData().getCustomData() == nullptr)
            return;

        CreatureLevListCustomData& customData = ptr.getRefData().getCustomData()->asCreatureLevListCustomData();
        MWWorld::Ptr creature = (customData.mSpawnActorId == -1)
            ? MWWorld::Ptr()
            : MWBase::Environment::get().getWorld()->searchPtrViaActorId(customData.mSpawnActorId);
        if (!creature.isEmpty())
            MWBase::Environment::get().getWorld()->adjustPosition(creature, force);
    }

    std::string_view CreatureLevList::getName(const MWWorld::ConstPtr& ptr) const
    {
        return {};
    }

    bool CreatureLevList::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        return false;
    }

    void CreatureLevList::respawn(const MWWorld::Ptr& ptr) const
    {
        ensureCustomData(ptr);

        CreatureLevListCustomData& customData = ptr.getRefData().getCustomData()->asCreatureLevListCustomData();
        if (customData.mSpawn)
            return;

        MWWorld::Ptr creature;
        if (customData.mSpawnActorId != -1)
        {
            creature = MWBase::Environment::get().getWorld()->searchPtrViaActorId(customData.mSpawnActorId);
            if (creature.isEmpty())
                creature = ptr.getCell()->getMovedActor(customData.mSpawnActorId);
        }
        if (!creature.isEmpty())
        {
            const MWMechanics::CreatureStats& creatureStats = creature.getClass().getCreatureStats(creature);
            if (creature.getCellRef().getCount() == 0)
                customData.mSpawn = true;
            else if (creatureStats.isDead())
            {
                const MWWorld::Store<ESM::GameSetting>& gmst
                    = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
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

    void CreatureLevList::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        ensureCustomData(ptr);

        CreatureLevListCustomData& customData = ptr.getRefData().getCustomData()->asCreatureLevListCustomData();
        if (!customData.mSpawn)
            return;

        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        const ESM::RefId& id = MWMechanics::getLevelledItem(
            store.get<ESM::CreatureLevList>().find(ptr.getCellRef().getRefId()), true, prng);

        if (!id.empty())
        {
            // Delete the previous creature
            if (customData.mSpawnActorId != -1)
            {
                MWWorld::Ptr creature
                    = MWBase::Environment::get().getWorld()->searchPtrViaActorId(customData.mSpawnActorId);
                if (!creature.isEmpty())
                    MWBase::Environment::get().getWorld()->deleteObject(creature);
                customData.mSpawnActorId = -1;
            }

            MWWorld::ManualRef manualRef(store, id);
            manualRef.getPtr().getCellRef().setPosition(ptr.getCellRef().getPosition());
            manualRef.getPtr().getCellRef().setScale(ptr.getCellRef().getScale());
            MWWorld::Ptr placed = MWBase::Environment::get().getWorld()->placeObject(
                manualRef.getPtr(), ptr.getCell(), ptr.getRefData().getPosition());
            customData.mSpawnActorId = placed.getClass().getCreatureStats(placed).getActorId();
            customData.mSpawn = false;
        }
        else
            customData.mSpawn = false;
    }

    void CreatureLevList::ensureCustomData(const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::unique_ptr<CreatureLevListCustomData> data = std::make_unique<CreatureLevListCustomData>();
            data->mSpawnActorId = -1;
            data->mSpawn = true;

            ptr.getRefData().setCustomData(std::move(data));
        }
    }

    void CreatureLevList::readAdditionalState(const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const
    {
        if (!state.mHasCustomState)
            return;

        ensureCustomData(ptr);
        CreatureLevListCustomData& customData = ptr.getRefData().getCustomData()->asCreatureLevListCustomData();
        const ESM::CreatureLevListState& levListState = state.asCreatureLevListState();
        customData.mSpawnActorId = levListState.mSpawnActorId;
        customData.mSpawn = levListState.mSpawn;
    }

    void CreatureLevList::writeAdditionalState(const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const
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
