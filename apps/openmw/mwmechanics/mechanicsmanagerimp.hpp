#ifndef GAME_MWMECHANICS_MECHANICSMANAGERIMP_H
#define GAME_MWMECHANICS_MECHANICSMANAGERIMP_H

#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/ptr.hpp"

#include "creaturestats.hpp"
#include "npcstats.hpp"
#include "objects.hpp"
#include "actors.hpp"

namespace Ogre
{
    class Vector3;
}

namespace MWWorld
{
    class CellStore;
}

namespace MWMechanics
{
    class MechanicsManager : public MWBase::MechanicsManager
    {
            MWWorld::Ptr mWatched;
            NpcStats mWatchedStats;
            bool mUpdatePlayer;
            bool mClassSelected;
            bool mRaceSelected;
            bool mAI;///< is AI active?

            Objects mObjects;
            Actors mActors;

        public:

            void buildPlayer();
            ///< build player according to stored class/race/birthsign information. Will
            /// default to the values of the ESM::NPC object, if no explicit information is given.

            MechanicsManager();

            virtual void add (const MWWorld::Ptr& ptr);
            ///< Register an object for management

            virtual void remove (const MWWorld::Ptr& ptr);
            ///< Deregister an object for management

            virtual void updateCell(const MWWorld::Ptr &old, const MWWorld::Ptr &ptr);
            ///< Moves an object to a new cell

            virtual void drop(const MWWorld::CellStore *cellStore);
            ///< Deregister all objects in the given cell.

            virtual void watchActor(const MWWorld::Ptr& ptr);
            ///< On each update look for changes in a previously registered actor and update the
            /// GUI accordingly.

            virtual void update (float duration, bool paused);
            ///< Update objects
            ///
            /// \param paused In game type does not currently advance (this usually means some GUI
            /// component is up).

            virtual void advanceTime (float duration);

            virtual void setPlayerName (const std::string& name);
            ///< Set player name.

            virtual void setPlayerRace (const std::string& id, bool male, const std::string &head, const std::string &hair);
            ///< Set player race.

            virtual void setPlayerBirthsign (const std::string& id);
            ///< Set player birthsign.

            virtual void setPlayerClass (const std::string& id);
            ///< Set player class to stock class.

            virtual void setPlayerClass (const ESM::Class& class_);
            ///< Set player class to custom class.

            virtual void restoreDynamicStats();
            ///< If the player is sleeping, this should be called every hour.

            virtual int getBarterOffer(const MWWorld::Ptr& ptr,int basePrice, bool buying);
            ///< This is used by every service to determine the price of objects given the trading skills of the player and NPC.

            virtual int getDerivedDisposition(const MWWorld::Ptr& ptr);
            ///< Calculate the diposition of an NPC toward the player.

            virtual int countDeaths (const std::string& id) const;
            ///< Return the number of deaths for actors with the given ID.

            virtual void getPersuasionDispositionChange (const MWWorld::Ptr& npc, PersuasionType type,
                float currentTemporaryDispositionDelta, bool& success, float& tempChange, float& permChange);
            void toLower(std::string npcFaction);
            ///< Perform a persuasion action on NPC

            /// Check if \a observer is potentially aware of \a ptr. Does not do a line of sight check!
            virtual bool awarenessCheck (const MWWorld::Ptr& ptr, const MWWorld::Ptr& observer);

            /**
             * @brief Commit a crime. If any actors witness the crime and report it,
             *        reportCrime will be called automatically.
             * @param arg Depends on \a type, e.g. for Theft, the value of the item that was stolen.
             * @return was the crime reported?
             */
            virtual bool commitCrime (const MWWorld::Ptr& ptr, const MWWorld::Ptr& victim,
                                      OffenseType type, int arg=0);
            virtual void reportCrime (const MWWorld::Ptr& ptr, const MWWorld::Ptr& victim,
                                      OffenseType type, int arg=0);
            /// Utility to check if taking this item is illegal and calling commitCrime if so
            virtual void itemTaken (const MWWorld::Ptr& ptr, const MWWorld::Ptr& item, int count);
            /// Attempt sleeping in a bed. If this is illegal, call commitCrime.
            /// @return was it illegal, and someone saw you doing it?
            virtual bool sleepInBed (const MWWorld::Ptr& ptr, const MWWorld::Ptr& bed);

        virtual void forceStateUpdate(const MWWorld::Ptr &ptr);

        virtual void playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number);
        virtual void skipAnimation(const MWWorld::Ptr& ptr);
        virtual bool checkAnimationPlaying(const MWWorld::Ptr& ptr, const std::string &groupName);

            /// Update magic effects for an actor. Usually done automatically once per frame, but if we're currently
            /// paused we may want to do it manually (after equipping permanent enchantment)
            virtual void updateMagicEffects (const MWWorld::Ptr& ptr);

        virtual void toggleAI();
        virtual bool isAIActive();
    };
}

#endif
