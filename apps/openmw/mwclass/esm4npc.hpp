#ifndef GAME_MWCLASS_ESM4ACTOR_H
#define GAME_MWCLASS_ESM4ACTOR_H

#include <components/esm4/loadcrea.hpp>
#include <components/esm4/loadnpc.hpp>

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/registeredclass.hpp"

#include "esm4base.hpp"

namespace MWClass
{
    class ESM4Npc final : public MWWorld::RegisteredClass<ESM4Npc>
    {
    public:
        ESM4Npc()
            : MWWorld::RegisteredClass<ESM4Npc>(ESM4::Npc::sRecordId)
        {
        }

        MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const override
        {
            const MWWorld::LiveCellRef<ESM4::Npc>* ref = ptr.get<ESM4::Npc>();
            return MWWorld::Ptr(cell.insert(ref), &cell);
        }

        void insertObjectRendering(const MWWorld::Ptr& ptr, const std::string& model,
            MWRender::RenderingInterface& renderingInterface) const override
        {
            renderingInterface.getObjects().insertNPC(ptr);
        }

        void insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override
        {
            insertObjectPhysics(ptr, model, rotation, physics);
        }

        void insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override
        {
            // ESM4Impl::insertObjectPhysics(ptr, getModel(ptr), rotation, physics);
        }

        bool hasToolTip(const MWWorld::ConstPtr& ptr) const override { return true; }
        MWGui::ToolTipInfo getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const override
        {
            return ESM4Impl::getToolTipInfo(getName(ptr), count);
        }

        std::string_view getModel(const MWWorld::ConstPtr& ptr) const override;
        std::string_view getName(const MWWorld::ConstPtr& ptr) const override;

        static const ESM4::Npc* getTraitsRecord(const MWWorld::Ptr& ptr);
        static const ESM4::Race* getRace(const MWWorld::Ptr& ptr);
        static bool isFemale(const MWWorld::Ptr& ptr);
        static const std::vector<const ESM4::Armor*>& getEquippedArmor(const MWWorld::Ptr& ptr);
        static const std::vector<const ESM4::Clothing*>& getEquippedClothing(const MWWorld::Ptr& ptr);

    private:
        static ESM4NpcCustomData& getCustomData(const MWWorld::ConstPtr& ptr);
    };
}

#endif // GAME_MWCLASS_ESM4ACTOR_H
