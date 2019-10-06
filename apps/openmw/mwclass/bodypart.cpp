#include "bodypart.hpp"

#include "../mwrender/renderinginterface.hpp"
#include "../mwrender/objects.hpp"

#include "../mwworld/cellstore.hpp"

namespace MWClass
{

    MWWorld::Ptr BodyPart::copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const
    {
        const MWWorld::LiveCellRef<ESM::BodyPart> *ref = ptr.get<ESM::BodyPart>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    void BodyPart::insertObjectRendering(const MWWorld::Ptr &ptr, const std::string &model, MWRender::RenderingInterface &renderingInterface) const
    {
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void BodyPart::insertObject(const MWWorld::Ptr &ptr, const std::string &model, MWPhysics::PhysicsSystem &physics) const
    {
    }

    std::string BodyPart::getName(const MWWorld::ConstPtr &ptr) const
    {
        return std::string();
    }

    bool BodyPart::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        return false;
    }

    void BodyPart::registerSelf()
    {
        std::shared_ptr<MWWorld::Class> instance (new BodyPart);

        registerClass (typeid (ESM::BodyPart).name(), instance);
    }

    std::string BodyPart::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::BodyPart> *ref = ptr.get<ESM::BodyPart>();

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

}
