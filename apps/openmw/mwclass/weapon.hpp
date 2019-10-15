#ifndef GAME_MWCLASS_WEAPON_H
#define GAME_MWCLASS_WEAPON_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Weapon : public MWWorld::Class
    {
            virtual MWWorld::Ptr
            copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const;

        public:

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const;

            virtual std::string getName (const MWWorld::ConstPtr& ptr) const;
            ///< \return name or ID; can return an empty string.

            virtual std::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const;
            ///< Generate action for activation

            virtual MWGui::ToolTipInfo getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            virtual bool hasItemHealth (const MWWorld::ConstPtr& ptr) const;
            ///< \return Item health data available?

            virtual int getItemMaxHealth (const MWWorld::ConstPtr& ptr) const;
            ///< Return item max health or throw an exception, if class does not have item health

            virtual std::string getScript (const MWWorld::ConstPtr& ptr) const;
            ///< Return name of the script attached to ptr

            virtual std::pair<std::vector<int>, bool> getEquipmentSlots (const MWWorld::ConstPtr& ptr) const;
            ///< \return first: Return IDs of the slot this object can be equipped in; second: can object
            /// stay stacked when equipped?

            virtual int getEquipmentSkill (const MWWorld::ConstPtr& ptr) const;
            /// Return the index of the skill this item corresponds to when equipped or -1, if there is
            /// no such skill.

            virtual int getValue (const MWWorld::ConstPtr& ptr) const;
            ///< Return trade value of the object. Throws an exception, if the object can't be traded.

            static void registerSelf();

            virtual std::string getUpSoundId (const MWWorld::ConstPtr& ptr) const;
            ///< Return the pick up sound Id

            virtual std::string getDownSoundId (const MWWorld::ConstPtr& ptr) const;
            ///< Return the put down sound Id

            virtual std::string getInventoryIcon (const MWWorld::ConstPtr& ptr) const;
            ///< Return name of inventory icon.

            virtual std::string getEnchantment (const MWWorld::ConstPtr& ptr) const;
            ///< @return the enchantment ID if the object is enchanted, otherwise an empty string

            virtual std::string applyEnchantment(const MWWorld::ConstPtr &ptr, const std::string& enchId, int enchCharge, const std::string& newName) const;
            ///< Creates a new record using \a ptr as template, with the given name and the given enchantment applied to it.

            virtual std::pair<int, std::string> canBeEquipped(const MWWorld::ConstPtr &ptr, const MWWorld::Ptr &npc) const;
            ///< Return 0 if player cannot equip item. 1 if can equip. 2 if it's twohanded weapon. 3 if twohanded weapon conflicts with that.
            ///  Second item in the pair specifies the error message

            virtual std::shared_ptr<MWWorld::Action> use (const MWWorld::Ptr& ptr, bool force=false) const;
            ///< Generate action for using via inventory menu

            virtual std::string getModel(const MWWorld::ConstPtr &ptr) const;

            virtual bool canSell (const MWWorld::ConstPtr& item, int npcServices) const;

            virtual float getWeight (const MWWorld::ConstPtr& ptr) const;

            virtual int getEnchantmentPoints (const MWWorld::ConstPtr& ptr) const;
    };
}

#endif
