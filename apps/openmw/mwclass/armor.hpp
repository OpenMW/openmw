#ifndef GAME_MWCLASS_ARMOR_H
#define GAME_MWCLASS_ARMOR_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Armor : public MWWorld::Class
    {
        public:

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const;

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual boost::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const;
            ///< Generate action for activation

            virtual bool hasItemHealth (const MWWorld::Ptr& ptr) const;
            ///< \return Item health data available?

            virtual int getItemMaxHealth (const MWWorld::Ptr& ptr) const;
            ///< Return item max health or throw an exception, if class does not have item health

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;
            ///< Return name of the script attached to ptr

            static void registerSelf();
    };
}

#endif
