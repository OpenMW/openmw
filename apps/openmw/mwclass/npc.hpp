#ifndef GAME_MWCLASS_NPC_H
#define GAME_MWCLASS_NPC_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Npc : public MWWorld::Class
    {
        public:
			
            virtual std::string getId (const MWWorld::Ptr& ptr) const;
            ///< Return ID of \a ptr

            virtual void insertObj (const MWWorld::Ptr& ptr, MWRender::CellRenderImp& cellRender,
                MWWorld::Environment& environment) const;
            ///< Add reference into a cell for rendering

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

            virtual MWWorld::ContainerStore<MWWorld::RefData>& getContainerStore (
                const MWWorld::Ptr& ptr) const;
            ///< Return container store

            virtual boost::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const;
            ///< Generate action for activation

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;
            ///< Return name of the script attached to ptr

            static void registerSelf();
    };
}

#endif
