#include "light4.hpp"
#include "classmodel.hpp"

#include "../mwphysics/physicssystem.hpp"
#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/ptr.hpp"

#include <components/esm4/loadligh.hpp>

namespace MWClass
{
    ESM4Light::ESM4Light()
        : MWWorld::RegisteredClass<ESM4Light>(ESM4::Light::sRecordId)
    {
    }

    void ESM4Light ::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Light>* ref = ptr.get<ESM4::Light>();

        // Insert even if model is empty, so that the light is added
        renderingInterface.getObjects().insertModel(ptr, model, !(ref->mBase->mData.flags & ESM4::Light::OffDefault));
    }

    void ESM4Light::insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
        MWPhysics::PhysicsSystem& physics) const
    {
        insertObjectPhysics(ptr, model, rotation, physics);
    }

    void ESM4Light::insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
        MWPhysics::PhysicsSystem& physics) const
    {
        physics.addObject(ptr, model, rotation, MWPhysics::CollisionType_World);
    }

    std::string ESM4Light::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM4::Light>(ptr);
    }

    std::string_view ESM4Light ::getName(const MWWorld::ConstPtr& ptr) const
    {
        return {};
    }

    bool ESM4Light::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        return false;
    }

    MWWorld::Ptr ESM4Light::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM4::Light>* ref = ptr.get<ESM4::Light>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }
}
