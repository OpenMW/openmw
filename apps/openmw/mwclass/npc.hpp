#ifndef GAME_MWCLASS_NPC_H
#define GAME_MWCLASS_NPC_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Npc : public MWWorld::Class
    {
            void ensureCustomData (const MWWorld::Ptr& ptr) const;

        public:

            virtual std::string getId (const MWWorld::Ptr& ptr) const;
            ///< Return ID of \a ptr

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const;

            virtual void enable (const MWWorld::Ptr& ptr, MWWorld::Environment& environment) const;
            ///< Enable reference; only does the non-rendering part

            virtual void disable (const MWWorld::Ptr& ptr, MWWorld::Environment& environment) const;
            ///< Enable reference; only does the non-rendering part

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual MWMechanics::CreatureStats& getCreatureStats (const MWWorld::Ptr& ptr) const;
            ///< Return creature stats

            virtual MWMechanics::NpcStats& getNpcStats (const MWWorld::Ptr& ptr) const;
            ///< Return NPC stats

            virtual MWWorld::ContainerStore& getContainerStore (const MWWorld::Ptr& ptr) const;
            ///< Return container store

            virtual MWWorld::InventoryStore& getInventoryStore (const MWWorld::Ptr& ptr) const;
            ///< Return inventory store

            virtual boost::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const;
            ///< Generate action for activation

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;
            ///< Return name of the script attached to ptr

           virtual void setForceStance (const MWWorld::Ptr& ptr, Stance stance, bool force) const;
            ///< Force or unforce a stance.

            virtual void setStance (const MWWorld::Ptr& ptr, Stance stance, bool set) const;
            ///< Set or unset a stance.

            virtual bool getStance (const MWWorld::Ptr& ptr, Stance stance, bool ignoreForce = false)
                const;
            ////< Check if a stance is active or not.

            virtual float getSpeed (const MWWorld::Ptr& ptr) const;
            ///< Return movement speed.

            virtual MWMechanics::Movement& getMovementSettings (const MWWorld::Ptr& ptr) const;
            ///< Return desired movement.

            virtual Ogre::Vector3 getMovementVector (const MWWorld::Ptr& ptr) const;
            ///< Return desired movement vector (determined based on movement settings,
            /// stance and stats).

            static void registerSelf();
    };
}

#endif
