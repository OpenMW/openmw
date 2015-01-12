
#include "static.hpp"

#include <components/esm/loadstat.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string Static::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM::Static>()->mBase->mId;
    }

    void Static::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM::Static> *ref =
            ptr.get<ESM::Static>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model, !ref->mBase->mPersistent);
        }
    }

    void Static::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string Static::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Static> *ref =
            ptr.get<ESM::Static>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Static::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void Static::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Static);

        registerClass (typeid (ESM::Static).name(), instance);
    }

    MWWorld::Ptr
    Static::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Static> *ref =
            ptr.get<ESM::Static>();

        return MWWorld::Ptr(&cell.get<ESM::Static>().insert(*ref), &cell);
    }
}
