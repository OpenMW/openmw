#ifndef GAME_MWCLASS_PROBE_H
#define GAME_MWCLASS_PROBE_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Probe : public MWWorld::Class
    {
            virtual MWWorld::Ptr
            copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const;

        public:

            /// Return ID of \a ptr
            virtual std::string getId (const MWWorld::Ptr& ptr) const;

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const;

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual boost::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const;
            ///< Generate action for activation

            virtual bool hasToolTip (const MWWorld::Ptr& ptr) const;
            ///< @return true if this object has a tooltip when focused (default implementation: false)

            virtual MWGui::ToolTipInfo getToolTipInfo (const MWWorld::Ptr& ptr) const;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;
            ///< Return name of the script attached to ptr

            virtual std::pair<std::vector<int>, bool> getEquipmentSlots (const MWWorld::Ptr& ptr) const;
            ///< \return first: Return IDs of the slot this object can be equipped in; second: can object
            /// stay stacked when equipped?

            virtual int getValue (const MWWorld::Ptr& ptr) const;
            ///< Return trade value of the object. Throws an exception, if the object can't be traded.

            static void registerSelf();

            virtual std::string getUpSoundId (const MWWorld::Ptr& ptr) const;
            ///< Return the pick up sound Id

            virtual std::string getDownSoundId (const MWWorld::Ptr& ptr) const;
            ///< Return the put down sound Id

            virtual std::string getInventoryIcon (const MWWorld::Ptr& ptr) const;
            ///< Return name of inventory icon.

            virtual boost::shared_ptr<MWWorld::Action> use (const MWWorld::Ptr& ptr)
                const;
            ///< Generate action for using via inventory menu

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;

            virtual bool canSell (const MWWorld::Ptr& item, int npcServices) const;

            virtual float getWeight (const MWWorld::Ptr& ptr) const;

            virtual int getItemMaxHealth (const MWWorld::Ptr& ptr) const;
            ///< Return item max health or throw an exception, if class does not have item health

            virtual bool hasItemHealth (const MWWorld::Ptr& ptr) const { return true; }
            ///< \return Item health data available? (default implementation: false)
    };
}

#endif
