#ifndef GAME_MWWORLD_CLASS_H
#define GAME_MWWORLD_CLASS_H

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

#include "action.hpp"
#include "containerstore.hpp"
#include "refdata.hpp"

namespace MWRender
{
    class CellRenderImp;
}

namespace MWMechanics
{
    struct CreatureStats;
    struct NpcStats;
}

namespace MWWorld
{
    class Ptr;
    class Environment;

    /// \brief Base class for referenceable esm records
    class Class
    {
            static std::map<std::string, boost::shared_ptr<Class> > sClasses;

            // not implemented
            Class (const Class&);
            Class& operator= (const Class&);

        protected:

            Class();

        public:

            /// NPC-stances.
            enum Stance
            {
                Run, Sneak, Combat
            };

            virtual ~Class();

            virtual std::string getId (const Ptr& ptr) const;
            ///< Return ID of \a ptr or throw an exception, if class does not support ID retrieval
            /// (default implementation: throw an exception)

            virtual void insertObj (const Ptr& ptr, MWRender::CellRenderImp& cellRender,
                MWWorld::Environment& environment) const;
            ///< Add reference into a cell for rendering (default implementation: don't render anything).

            virtual void enable (const Ptr& ptr, MWWorld::Environment& environment) const;
            ///< Enable reference; only does the non-rendering part (default implementation: ignore)
            /// \attention This is not the same as the script instruction with the same name. References
            /// should only be enabled while in an active cell.

            virtual void disable (const Ptr& ptr, MWWorld::Environment& environment) const;
            ///< Enable reference; only does the non-rendering part (default implementation: ignore)
            /// \attention This is not the same as the script instruction with the same name. References
            /// should only be enabled while in an active cell.

            virtual std::string getName (const Ptr& ptr) const = 0;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual MWMechanics::CreatureStats& getCreatureStats (const Ptr& ptr) const;
            ///< Return creature stats or throw an exception, if class does not have creature stats
            /// (default implementation: throw an exceoption)

            virtual MWMechanics::NpcStats& getNpcStats (const Ptr& ptr) const;
            ///< Return NPC stats or throw an exception, if class does not have NPC stats
            /// (default implementation: throw an exceoption)

            virtual bool hasItemHealth (const Ptr& ptr) const;
            ///< \return Item health data available? (default implementation: false)

            virtual int getItemMaxHealth (const Ptr& ptr) const;
            ///< Return item max health or throw an exception, if class does not have item health
            /// (default implementation: throw an exceoption)

            virtual boost::shared_ptr<Action> activate (const Ptr& ptr, const Ptr& actor,
                const Environment& environment) const;
            ///< Generate action for activation (default implementation: return a null action).

            virtual boost::shared_ptr<Action> use (const Ptr& ptr, const Environment& environment)
                const;
            ///< Generate action for using via inventory menu (default implementation: return a
            /// null action).

            virtual ContainerStore<RefData>& getContainerStore (const Ptr& ptr) const;
            ///< Return container store or throw an exception, if class does not have a
            /// container store (default implementation: throw an exceoption)

            virtual void insertIntoContainer (const Ptr& ptr, ContainerStore<RefData>& containerStore)
                const;
            ///< Insert into a container or throw an exception, if class does not support inserting into
            /// a container.

            virtual void lock (const Ptr& ptr, int lockLevel) const;
            ///< Lock object (default implementation: throw an exception)

            virtual void unlock (const Ptr& ptr) const;
            ///< Unlock object (default implementation: throw an exception)

            virtual std::string getScript (const Ptr& ptr) const;
            ///< Return name of the script attached to ptr (default implementation: return an empty
            /// string).

            virtual void setForceStance (const Ptr& ptr, Stance stance, bool force) const;
            ///< Force or unforce a stance.

            virtual void setStance (const Ptr& ptr, Stance stance, bool set) const;
            ///< Set or unset a stance.

            virtual bool getStance (const Ptr& ptr, Stance stance, bool ignoreForce = false) const;
            ////< Check if a stance is active or not.

            virtual float getSpeed (const Ptr& ptr) const;
            ///< Return movement speed.

            static const Class& get (const std::string& key);
            ///< If there is no class for this \a key, an exception is thrown.

            static const Class& get (const Ptr& ptr);
            ///< If there is no class for this pointer, an exception is thrown.

            static void registerClass (const std::string& key,  boost::shared_ptr<Class> instance);
    };
}

#endif
