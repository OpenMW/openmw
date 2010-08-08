#ifndef GAME_MWCLASS_ARMOR_H
#define GAME_MWCLASS_ARMOR_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Armor : public MWWorld::Class
    {
        public:

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

            virtual void insertIntoContainer (const MWWorld::Ptr& ptr,
                MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const;
            ///< Insert into a containe

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;
            ///< Return name of the script attached to ptr

            static void registerSelf();
    };
}

#endif
