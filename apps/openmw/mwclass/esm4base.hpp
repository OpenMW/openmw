#ifndef GAME_MWCLASS_ESM4BASE_H
#define GAME_MWCLASS_ESM4BASE_H

#include <components/esm4/inventory.hpp>
#include <components/esm4/loadstat.hpp>
#include <components/esm4/loadtree.hpp>
#include <components/misc/strings/algorithm.hpp>

#include "../mwbase/environment.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
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

        // We don't handle ESM4 player stats yet, so for resolving levelled object we use an arbitrary number.
        constexpr int sDefaultLevel = 5;

        template <class LevelledRecord, class TargetRecord>
        const TargetRecord* resolveLevelled(const ESM::RefId& id, int level = sDefaultLevel)
        {
            if (id.empty())
                return nullptr;
            const MWWorld::ESMStore* esmStore = MWBase::Environment::get().getESMStore();
            const auto& targetStore = esmStore->get<TargetRecord>();
            const TargetRecord* res = targetStore.search(id);
            if (res)
                return res;
            const LevelledRecord* lvlRec = esmStore->get<LevelledRecord>().search(id);
            if (!lvlRec)
                return nullptr;
            for (const ESM4::LVLO& obj : lvlRec->mLvlObject)
            {
                ESM::RefId candidateId = ESM::FormId::fromUint32(obj.item);
                if (candidateId == id)
                    continue;
                const TargetRecord* candidate = resolveLevelled<LevelledRecord, TargetRecord>(candidateId, level);
                if (candidate && (!res || obj.level <= level))
                    res = candidate;
            }
            return res;
        }
    }

    // Base for many ESM4 Classes
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
            ESM4Impl::insertObjectPhysics(ptr, model, rotation, physics);
        }

        bool hasToolTip(const MWWorld::ConstPtr& ptr) const override { return false; }

        std::string_view getName(const MWWorld::ConstPtr& ptr) const override { return {}; }

        std::string_view getModel(const MWWorld::ConstPtr& ptr) const override
        {
            std::string_view model = getClassModel<Record>(ptr);

            // Hide meshes meshes/marker/* and *LOD.nif in ESM4 cells. It is a temporarty hack.
            // Needed because otherwise LOD meshes are rendered on top of normal meshes.
            // TODO: Figure out a better way find markers and LOD meshes; show LOD only outside of active grid.
            if (model.empty() || Misc::StringUtils::ciStartsWith(model, "marker")
                || Misc::StringUtils::ciEndsWith(model, "lod.nif"))
                return {};

            return model;
        }
    };

    class ESM4Static final : public MWWorld::RegisteredClass<ESM4Static, ESM4Base<ESM4::Static>>
    {
        friend MWWorld::RegisteredClass<ESM4Static, ESM4Base<ESM4::Static>>;
        ESM4Static()
            : MWWorld::RegisteredClass<ESM4Static, ESM4Base<ESM4::Static>>(ESM4::Static::sRecordId)
        {
        }
    };

    class ESM4Tree final : public MWWorld::RegisteredClass<ESM4Tree, ESM4Base<ESM4::Tree>>
    {
        friend MWWorld::RegisteredClass<ESM4Tree, ESM4Base<ESM4::Tree>>;
        ESM4Tree()
            : MWWorld::RegisteredClass<ESM4Tree, ESM4Base<ESM4::Tree>>(ESM4::Tree::sRecordId)
        {
        }
    };

    // For records with `mFullName` that should be shown as a tooltip.
    // All objects with a tooltip can be activated (activation can be handled in Lua).
    template <typename Record>
    class ESM4Named : public MWWorld::RegisteredClass<ESM4Named<Record>, ESM4Base<Record>>
    {
    public:
        ESM4Named()
            : MWWorld::RegisteredClass<ESM4Named, ESM4Base<Record>>(Record::sRecordId)
        {
        }

        std::string_view getName(const MWWorld::ConstPtr& ptr) const override
        {
            return ptr.get<Record>()->mBase->mFullName;
        }

        MWGui::ToolTipInfo getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const override
        {
            return ESM4Impl::getToolTipInfo(getName(ptr), count);
        }

        bool hasToolTip(const MWWorld::ConstPtr& ptr) const override { return !getName(ptr).empty(); }
    };
}

#endif // GAME_MWCLASS_ESM4BASE_H
