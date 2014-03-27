
#include "creaturelevlist.hpp"

#include <components/esm/loadlevlist.hpp>

#include "../mwmechanics/levelledlist.hpp"

#include "../mwworld/customdata.hpp"

namespace
{
    struct CreatureLevListCustomData : public MWWorld::CustomData
    {
        // TODO: save the creature we spawned here
        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *CreatureLevListCustomData::clone() const
    {
        return new CreatureLevListCustomData (*this);
    }
}

namespace MWClass
{
    std::string CreatureLevList::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void CreatureLevList::registerSelf()
    {
        boost::shared_ptr<Class> instance (new CreatureLevList);

        registerClass (typeid (ESM::CreatureLevList).name(), instance);
    }

    void CreatureLevList::insertObjectRendering(const MWWorld::Ptr &ptr, MWRender::RenderingInterface &renderingInterface) const
    {
        ensureCustomData(ptr);
    }

    void CreatureLevList::ensureCustomData(const MWWorld::Ptr &ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<CreatureLevListCustomData> data (new CreatureLevListCustomData);

            MWWorld::LiveCellRef<ESM::CreatureLevList> *ref =
                ptr.get<ESM::CreatureLevList>();

            std::string id = MWMechanics::getLevelledItem(ref->mBase, true);

            if (!id.empty())
            {
                const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
                MWWorld::ManualRef ref(store, id);
                ref.getPtr().getCellRef().mPos = ptr.getCellRef().mPos;
                // TODO: hold on to this for respawn purposes later
                MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(), ptr.getCell() , ptr.getCellRef().mPos);
            }

            ptr.getRefData().setCustomData(data.release());
        }
    }
}
