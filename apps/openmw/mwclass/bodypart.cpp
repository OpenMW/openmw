#include "bodypart.hpp"

#include <components/esm3/loadbody.hpp>

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwworld/cellstore.hpp"

#include "classmodel.hpp"

namespace MWClass
{
    BodyPart::BodyPart()
        : MWWorld::RegisteredClass<BodyPart>(ESM::BodyPart::sRecordId)
    {
    }

    MWWorld::Ptr BodyPart::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM::BodyPart>* ref = ptr.get<ESM::BodyPart>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    void BodyPart::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty())
        {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    std::string_view BodyPart::getName(const MWWorld::ConstPtr& ptr) const
    {
        return {};
    }

    bool BodyPart::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        return false;
    }

    std::string_view BodyPart::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM::BodyPart>(ptr);
    }

}
