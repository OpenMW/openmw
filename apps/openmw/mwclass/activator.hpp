#ifndef GAME_MWCLASS_ACTIVATOR_H
#define GAME_MWCLASS_ACTIVATOR_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Activator : public MWWorld::Class
    {

            MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const override;

            static int getSndGenTypeFromName(const std::string &name);

        public:

            void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const override;
            ///< Add reference into a cell for rendering

            void insertObject(const MWWorld::Ptr& ptr, const std::string& model, osg::Quat rotation, MWPhysics::PhysicsSystem& physics, bool skipAnimated = false) const override;

            std::string getName (const MWWorld::ConstPtr& ptr) const override;
            ///< \return name or ID; can return an empty string.

            bool hasToolTip (const MWWorld::ConstPtr& ptr) const override;
            ///< @return true if this object has a tooltip when focused (default implementation: true)

            bool allowTelekinesis(const MWWorld::ConstPtr& ptr) const override;
            ///< Return whether this class of object can be activated with telekinesis

            MWGui::ToolTipInfo getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const override;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            std::string getScript (const MWWorld::ConstPtr& ptr) const override;
            ///< Return name of the script attached to ptr

            std::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const override;
            ///< Generate action for activation

            static void registerSelf();

            std::string getModel(const MWWorld::ConstPtr &ptr) const override;

            bool useAnim() const override;
            ///< Whether or not to use animated variant of model (default false)

            bool isActivator() const override;

            std::string getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const override;
    };
}

#endif
