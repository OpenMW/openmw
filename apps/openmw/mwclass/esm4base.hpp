#ifndef GAME_MWCLASS_ESM4BASE_H
#define GAME_MWCLASS_ESM4BASE_H

#include <components/esm4/loadstat.hpp>

#include "../mwgui/tooltips.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/registeredclass.hpp"

#include "classmodel.hpp"

namespace MWClass
{

    namespace ESM4Impl
    {
        void insertObjectRendering(
            const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface);
        void insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics);
        MWGui::ToolTipInfo getToolTipInfo(std::string_view name, int count);
    }

    // Base for all ESM4 Classes
    template <typename Record>
    class ESM4Base : public MWWorld::Class
    {

        MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const override
        {
            const MWWorld::LiveCellRef<Record>* ref = ptr.get<Record>();
            return MWWorld::Ptr(cell.insert(ref), &cell);
        }

    protected:
        explicit ESM4Base(unsigned type)
            : MWWorld::Class(type)
        {
        }

    public:
        void insertObjectRendering(const MWWorld::Ptr& ptr, const std::string& model,
            MWRender::RenderingInterface& renderingInterface) const override
        {
            const MWWorld::LiveCellRef<Record>* ref = ptr.get<Record>();
            if (ref->mBase->mFlags & ESM4::Rec_Marker)
                return;
            ESM4Impl::insertObjectRendering(ptr, model, renderingInterface);
        }

        void insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override
        {
            insertObjectPhysics(ptr, model, rotation, physics);
        }

        void insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override
        {
            const MWWorld::LiveCellRef<Record>* ref = ptr.get<Record>();
            if (ref->mBase->mFlags & ESM4::Rec_Marker)
                return;
            ESM4Impl::insertObjectPhysics(ptr, model, rotation, physics);
        }

        bool hasToolTip(const MWWorld::ConstPtr& ptr) const override { return false; }

        std::string_view getName(const MWWorld::ConstPtr& ptr) const override { return ""; }

        std::string getModel(const MWWorld::ConstPtr& ptr) const override { return getClassModel<Record>(ptr); }
    };

    class ESM4Static final : public MWWorld::RegisteredClass<ESM4Static, ESM4Base<ESM4::Static>>
    {
        friend MWWorld::RegisteredClass<ESM4Static, ESM4Base<ESM4::Static>>;
        ESM4Static()
            : MWWorld::RegisteredClass<ESM4Static, ESM4Base<ESM4::Static>>(ESM4::Static::sRecordId)
        {
        }
    };

    // For records with `mFullName` that should be shown as a tooltip.
    // All objects with a tooltip can be activated (activation can be handled in Lua).
    template <typename Record>
    class ESM4Named : public MWWorld::RegisteredClass<ESM4Named<Record>, ESM4Base<Record>>
    {
    public:
        friend MWWorld::RegisteredClass<ESM4Named, ESM4Base<Record>>;

        ESM4Named()
            : MWWorld::RegisteredClass<ESM4Named, ESM4Base<Record>>(Record::sRecordId)
        {
        }

    public:
        bool hasToolTip(const MWWorld::ConstPtr& ptr) const override { return true; }

        MWGui::ToolTipInfo getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const override
        {
            return ESM4Impl::getToolTipInfo(ptr.get<Record>()->mBase->mFullName, count);
        }

        std::string_view getName(const MWWorld::ConstPtr& ptr) const override
        {
            return ptr.get<Record>()->mBase->mFullName;
        }
    };
}

#endif // GAME_MWCLASS_ESM4BASE_H
