#ifndef GAME_MWCLASS_APPARATUS_H
#define GAME_MWCLASS_APPARATUS_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Apparatus : public MWWorld::Class
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

            std::string getScript (const MWWorld::ConstPtr& ptr) const override;
            ///< Return name of the script attached to ptr

            int getValue (const MWWorld::ConstPtr& ptr) const override;
            ///< Return trade value of the object. Throws an exception, if the object can't be traded.

            MWGui::ToolTipInfo getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const override;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            static void registerSelf();

            std::string getUpSoundId (const MWWorld::ConstPtr& ptr) const override;
            ///< Return the pick up sound Id

            std::string getDownSoundId (const MWWorld::ConstPtr& ptr) const override;
            ///< Return the put down sound Id

            std::string getInventoryIcon (const MWWorld::ConstPtr& ptr) const override;
            ///< Return name of inventory icon.

            std::shared_ptr<MWWorld::Action> use (const MWWorld::Ptr& ptr, bool force=false) const override;
            ///< Generate action for using via inventory menu

            std::string getModel(const MWWorld::ConstPtr &ptr) const override;

            bool canSell (const MWWorld::ConstPtr& item, int npcServices) const override;
    };
}

#endif
