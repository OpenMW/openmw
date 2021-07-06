#ifndef GAME_MWCLASS_ARMOR_H
#define GAME_MWCLASS_ARMOR_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Armor : public MWWorld::Class
    {
            MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const override;

        public:

            float getWeight (const MWWorld::ConstPtr& ptr) const override;

            void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const override;
            ///< Add reference into a cell for rendering

            std::string getName (const MWWorld::ConstPtr& ptr) const override;
            ///< \return name or ID; can return an empty string.

            std::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const override;
            ///< Generate action for activation

            bool hasItemHealth (const MWWorld::ConstPtr& ptr) const override;
            ///< \return Item health data available?

            int getItemMaxHealth (const MWWorld::ConstPtr& ptr) const override;
            ///< Return item max health or throw an exception, if class does not have item health

            std::string getScript (const MWWorld::ConstPtr& ptr) const override;
            ///< Return name of the script attached to ptr

            std::pair<std::vector<int>, bool> getEquipmentSlots (const MWWorld::ConstPtr& ptr) const override;
            ///< \return first: Return IDs of the slot this object can be equipped in; second: can object
            /// stay stacked when equipped?

            int getEquipmentSkill (const MWWorld::ConstPtr& ptr) const override;
            /// Return the index of the skill this item corresponds to when equipped or -1, if there is
            /// no such skill.

            MWGui::ToolTipInfo getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const override;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            int getValue (const MWWorld::ConstPtr& ptr) const override;
            ///< Return trade value of the object. Throws an exception, if the object can't be traded.

            static void registerSelf();

            std::string getUpSoundId (const MWWorld::ConstPtr& ptr) const override;
            ///< Return the pick up sound Id

            std::string getDownSoundId (const MWWorld::ConstPtr& ptr) const override;
            ///< Return the put down sound Id

            std::string getInventoryIcon (const MWWorld::ConstPtr& ptr) const override;
            ///< Return name of inventory icon.

            std::string getEnchantment (const MWWorld::ConstPtr& ptr) const override;
            ///< @return the enchantment ID if the object is enchanted, otherwise an empty string

            std::string applyEnchantment(const MWWorld::ConstPtr &ptr, const std::string& enchId, int enchCharge, const std::string& newName) const override;
            ///< Creates a new record using \a ptr as template, with the given name and the given enchantment applied to it.

            std::pair<int, std::string> canBeEquipped(const MWWorld::ConstPtr &ptr, const MWWorld::Ptr &npc) const override;
            ///< Return 0 if player cannot equip item. 1 if can equip. 2 if it's twohanded weapon. 3 if twohanded weapon conflicts with that. \n
            ///  Second item in the pair specifies the error message

            std::shared_ptr<MWWorld::Action> use (const MWWorld::Ptr& ptr, bool force=false) const override;
            ///< Generate action for using via inventory menu

            std::string getModel(const MWWorld::ConstPtr &ptr) const override;

            int getEnchantmentPoints (const MWWorld::ConstPtr& ptr) const override;

            bool canSell (const MWWorld::ConstPtr& item, int npcServices) const override;

            /// Get the effective armor rating, factoring in the actor's skills, for the given armor.
            float getEffectiveArmorRating(const MWWorld::ConstPtr& armor, const MWWorld::Ptr& actor) const override;
    };
}

#endif
