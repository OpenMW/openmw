#ifndef GAME_MWCLASS_STATIC_H
#define GAME_MWCLASS_STATIC_H

#include "../mwworld/cellstore.hpp"
#include "../mwworld/registeredclass.hpp"

#include "classmodel.hpp"

namespace MWClass
{
    class Static : public MWWorld::RegisteredClass<Static>
    {
        friend MWWorld::RegisteredClass<Static>;

        Static();

        MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const override;

    public:
        void insertObjectRendering(const MWWorld::Ptr& ptr, const std::string& model,
            MWRender::RenderingInterface& renderingInterface) const override;
        ///< Add reference into a cell for rendering

        void insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override;
        void insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override;

        std::string_view getName(const MWWorld::ConstPtr& ptr) const override;
        ///< \return name or ID; can return an empty string.

        bool hasToolTip(const MWWorld::ConstPtr& ptr) const override;
        ///< @return true if this object has a tooltip when focused (default implementation: true)

        std::string getModel(const MWWorld::ConstPtr& ptr) const override;
    };

    namespace ESM4StaticImpl
    {
        void insertObjectRendering(
            const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface);
        void insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics);
    }

    // Templated because it is used as a dummy MWClass implementation for several ESM4 recors
    template <typename Record>
    class ESM4Static : public MWWorld::RegisteredClass<ESM4Static<Record>>
    {
        friend MWWorld::RegisteredClass<ESM4Static>;

        ESM4Static()
            : MWWorld::RegisteredClass<ESM4Static>(Record::sRecordId)
        {
        }

        MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const override
        {
            const MWWorld::LiveCellRef<Record>* ref = ptr.get<Record>();
            return MWWorld::Ptr(cell.insert(ref), &cell);
        }

    public:
        void insertObjectRendering(const MWWorld::Ptr& ptr, const std::string& model,
            MWRender::RenderingInterface& renderingInterface) const override
        {
            ESM4StaticImpl::insertObjectRendering(ptr, model, renderingInterface);
        }

        void insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override
        {
            insertObjectPhysics(ptr, model, rotation, physics);
        }
        void insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override
        {
            ESM4StaticImpl::insertObjectPhysics(ptr, model, rotation, physics);
        }

        std::string_view getName(const MWWorld::ConstPtr& ptr) const override { return ""; }
        ///< \return name or ID; can return an empty string.

        bool hasToolTip(const MWWorld::ConstPtr& ptr) const override { return false; }
        ///< @return true if this object has a tooltip when focused (default implementation: true)

        std::string getModel(const MWWorld::ConstPtr& ptr) const override { return getClassModel<Record>(ptr); }
    };
}

#endif
