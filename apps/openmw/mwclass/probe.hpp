#ifndef GAME_MWCLASS_PROBE_H
#define GAME_MWCLASS_PROBE_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Probe : public MWWorld::Class
    {
            MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const override;

        public:

            void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const override;
            ///< Add reference into a cell for rendering

            std::string getName (const MWWorld::ConstPtr& ptr) const override;
            ///< \return name or ID; can return an empty string.

            std::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const override;
            ///< Generate action for activation

            MWGui::ToolTipInfo getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const override;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            std::string getScript (const MWWorld::ConstPtr& ptr) const override;
            ///< Return name of the script attached to ptr

            std::pair<std::vector<int>, bool> getEquipmentSlots (const MWWorld::ConstPtr& ptr) const override;
            ///< \return first: Return IDs of the slot this object can be equipped in; second: can object
            /// stay stacked when equipped?

            int getValue (const MWWorld::ConstPtr& ptr) const override;
            ///< Return trade value of the object. Throws an exception, if the object can't be traded.

            static void registerSelf();

            std::string getUpSoundId (const MWWorld::ConstPtr& ptr) const override;
            ///< Return the pick up sound Id

            std::string getDownSoundId (const MWWorld::ConstPtr& ptr) const override;
            ///< Return the put down sound Id

            std::string getInventoryIcon (const MWWorld::ConstPtr& ptr) const override;
            ///< Return name of inventory icon.

            std::pair<int, std::string> canBeEquipped(const MWWorld::ConstPtr &ptr, const MWWorld::Ptr &npc) const override;

            std::shared_ptr<MWWorld::Action> use (const MWWorld::Ptr& ptr, bool force=false) const override;
            ///< Generate action for using via inventory menu

            std::string getModel(const MWWorld::ConstPtr &ptr) const override;

            bool canSell (const MWWorld::ConstPtr& item, int npcServices) const override;

            float getWeight (const MWWorld::ConstPtr& ptr) const override;

            int getItemMaxHealth (const MWWorld::ConstPtr& ptr) const override;
            ///< Return item max health or throw an exception, if class does not have item health

            bool hasItemHealth (const MWWorld::ConstPtr& ptr) const override { return true; }
            ///< \return Item health data available? (default implementation: false)
    };
}

#endif
