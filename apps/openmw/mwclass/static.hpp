#ifndef GAME_MWCLASS_STATIC_H
#define GAME_MWCLASS_STATIC_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Static : public MWWorld::Class
    {
            MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const override;

        public:

            void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const override;
            ///< Add reference into a cell for rendering

            void insertObject(const MWWorld::Ptr& ptr, const std::string& model, osg::Quat rotation, MWPhysics::PhysicsSystem& physics, bool skipAnimated = false) const override;

            std::string getName (const MWWorld::ConstPtr& ptr) const override;
            ///< \return name or ID; can return an empty string.

            bool hasToolTip (const MWWorld::ConstPtr& ptr) const override;
            ///< @return true if this object has a tooltip when focused (default implementation: true)

            static void registerSelf();

            std::string getModel(const MWWorld::ConstPtr &ptr) const override;
    };
}

#endif
