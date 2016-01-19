#include "creaturelevlist.hpp"

#include <components/esm/loadlevlist.hpp>
#include <components/esm/creaturelevliststate.hpp>

#include "../mwmechanics/levelledlist.hpp"

#include "../mwworld/customdata.hpp"

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

    void CreatureLevList::respawn(const MWWorld::Ptr &ptr) const
    {
        ensureCustomData(ptr);

        CreatureLevListCustomData& customData = ptr.getRefData().getCustomData()->asCreatureLevListCustomData();
        customData.mSpawn = true;
    }

    void CreatureLevList::registerSelf()
    {
        boost::shared_ptr<Class> instance (new CreatureLevList);

        registerClass (typeid (ESM::CreatureLevList).name(), instance);
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
            MWWorld::ManualRef ref(store, id);
            ref.getPtr().getCellRef().setPosition(ptr.getCellRef().getPosition());
            MWWorld::Ptr placed = MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(), ptr.getCell() , ptr.getCellRef().getPosition());
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
            std::auto_ptr<CreatureLevListCustomData> data (new CreatureLevListCustomData);
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

        const ESM::CreatureLevListState& state2 = dynamic_cast<const ESM::CreatureLevListState&> (state);

        ensureCustomData(ptr);
        CreatureLevListCustomData& customData = ptr.getRefData().getCustomData()->asCreatureLevListCustomData();
        customData.mSpawnActorId = state2.mSpawnActorId;
        customData.mSpawn = state2.mSpawn;
    }

    void CreatureLevList::writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state)
        const
    {
        ESM::CreatureLevListState& state2 = dynamic_cast<ESM::CreatureLevListState&> (state);

        if (!ptr.getRefData().getCustomData())
        {
            state.mHasCustomState = false;
            return;
        }

        const CreatureLevListCustomData& customData = ptr.getRefData().getCustomData()->asCreatureLevListCustomData();
        state2.mSpawnActorId = customData.mSpawnActorId;
        state2.mSpawn = customData.mSpawn;
    }
}
