#ifndef GAME_MWCLASS_MISC_H
#define GAME_MWCLASS_MISC_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Miscellaneous : public MWWorld::Class
    {
        public:

            virtual MWWorld::Ptr copyToCell(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell, int count) const;

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

            virtual std::string getScript (const MWWorld::ConstPtr& ptr) const;
            ///< Return name of the script attached to ptr

            virtual int getValue (const MWWorld::ConstPtr& ptr) const;
            ///< Return trade value of the object. Throws an exception, if the object can't be traded.

            static void registerSelf();

            virtual std::string getUpSoundId (const MWWorld::ConstPtr& ptr) const;
            ///< Return the pick up sound Id

            virtual std::string getDownSoundId (const MWWorld::ConstPtr& ptr) const;
            ///< Return the put down sound Id

            virtual std::string getInventoryIcon (const MWWorld::ConstPtr& ptr) const;
            ///< Return name of inventory icon.

            virtual std::string getModel(const MWWorld::ConstPtr &ptr) const;

            virtual std::shared_ptr<MWWorld::Action> use (const MWWorld::Ptr& ptr, bool force=false) const;
            ///< Generate action for using via inventory menu

            virtual float getWeight (const MWWorld::ConstPtr& ptr) const;

            virtual bool canSell (const MWWorld::ConstPtr& item, int npcServices) const;

            virtual bool isKey (const MWWorld::ConstPtr &ptr) const;

            virtual bool isGold (const MWWorld::ConstPtr& ptr) const;
    };
}

#endif
