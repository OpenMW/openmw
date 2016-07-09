#ifndef GAME_MWCLASS_BODYPART_H
#define GAME_MWCLASS_BODYPART_H

#include "../mwworld/class.hpp"

namespace MWClass
{

    class BodyPart : public MWWorld::Class
    {
        virtual MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const;

    public:

        virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;
        ///< Add reference into a cell for rendering

        virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const;

        virtual std::string getName (const MWWorld::ConstPtr& ptr) const;
        ///< \return name (the one that is to be presented to the user; not the internal one);
        /// can return an empty string.

        static void registerSelf();

        virtual std::string getModel(const MWWorld::ConstPtr &ptr) const;
    };

}

#endif
