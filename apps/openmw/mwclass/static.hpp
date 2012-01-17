#ifndef GAME_MWCLASS_STATIC_H
#define GAME_MWCLASS_STATIC_H

#include "../mwworld/class.hpp"
#include "../mwrender/objects.hpp"

namespace MWClass
{
    class Static : public MWWorld::Class
    {
        public:

             virtual void insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const;

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            static void registerSelf();
    };
}

#endif
