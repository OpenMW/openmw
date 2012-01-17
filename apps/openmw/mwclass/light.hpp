#ifndef GAME_MWCLASS_LIGHT_H
#define GAME_MWCLASS_LIGHT_H

#include "../mwworld/class.hpp"
#include "../mwrender/objects.hpp"

namespace MWClass
{
    class Light : public MWWorld::Class
    {
        public:

             virtual void insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const;

            virtual void enable (const MWWorld::Ptr& ptr, MWWorld::Environment& environment) const;
            ///< Enable reference; only does the non-rendering part
            /// \attention This is not the same as the script instruction with the same name. References
            /// should only be enabled while in an active cell.

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual boost::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const;
            ///< Generate action for activation

            virtual void insertIntoContainer (const MWWorld::Ptr& ptr,
                MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const;
            ///< Insert into a containe

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;
            ///< Return name of the script attached to ptr

            static void registerSelf();
    };
}

#endif
