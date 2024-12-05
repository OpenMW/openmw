#ifndef GAME_MWCLASS_LIGHT_H
#define GAME_MWCLASS_LIGHT_H

#include "../mwworld/registeredclass.hpp"

namespace MWClass
{
    class Light : public MWWorld::RegisteredClass<Light>
    {
        friend MWWorld::RegisteredClass<Light>;

        Light();

        MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const override;

    public:
        void insertObjectRendering(const MWWorld::Ptr& ptr, const std::string& model,
            MWRender::RenderingInterface& renderingInterface) const override;
        ///< Add reference into a cell for rendering

        void insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override;
        void insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override;

        bool useAnim() const override;

        std::string_view getName(const MWWorld::ConstPtr& ptr) const override;
        ///< \return name or ID; can return an empty string.

        bool hasToolTip(const MWWorld::ConstPtr& ptr) const override;
        ///< @return true if this object has a tooltip when focused (default implementation: true)

        MWGui::ToolTipInfo getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const override;
        ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

        bool showsInInventory(const MWWorld::ConstPtr& ptr) const override;

        bool isItem(const MWWorld::ConstPtr&) const override;

        std::unique_ptr<MWWorld::Action> activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const override;
        ///< Generate action for activation

        ESM::RefId getScript(const MWWorld::ConstPtr& ptr) const override;
        ///< Return name of the script attached to ptr

        std::pair<std::vector<int>, bool> getEquipmentSlots(const MWWorld::ConstPtr& ptr) const override;
        ///< \return first: Return IDs of the slot this object can be equipped in; second: can object
        /// stay stacked when equipped?

        int getValue(const MWWorld::ConstPtr& ptr) const override;
        ///< Return trade value of the object. Throws an exception, if the object can't be traded.

        const ESM::RefId& getUpSoundId(const MWWorld::ConstPtr& ptr) const override;
        ///< Return the pick up sound Id

        const ESM::RefId& getDownSoundId(const MWWorld::ConstPtr& ptr) const override;
        ///< Return the put down sound Id

        const std::string& getInventoryIcon(const MWWorld::ConstPtr& ptr) const override;
        ///< Return name of inventory icon.

        std::unique_ptr<MWWorld::Action> use(const MWWorld::Ptr& ptr, bool force = false) const override;
        ///< Generate action for using via inventory menu

        void setRemainingUsageTime(const MWWorld::Ptr& ptr, float duration) const override;
        ///< Sets the remaining duration of the object.

        float getRemainingUsageTime(const MWWorld::ConstPtr& ptr) const override;
        ///< Returns the remaining duration of the object.

        std::string_view getModel(const MWWorld::ConstPtr& ptr) const override;

        float getWeight(const MWWorld::ConstPtr& ptr) const override;

        bool canSell(const MWWorld::ConstPtr& item, int npcServices) const override;

        std::pair<int, std::string_view> canBeEquipped(
            const MWWorld::ConstPtr& ptr, const MWWorld::Ptr& npc) const override;

        ESM::RefId getSound(const MWWorld::ConstPtr& ptr) const override;
    };

}

#endif
