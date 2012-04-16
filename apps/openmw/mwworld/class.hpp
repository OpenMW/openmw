#ifndef GAME_MWWORLD_CLASS_H
#define GAME_MWWORLD_CLASS_H

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "action.hpp"
#include "refdata.hpp"
#include "physicssystem.hpp"

#include "../mwrender/renderinginterface.hpp"
#include "../mwgui/tooltips.hpp"

namespace Ogre
{
    class Vector3;
}

namespace MWRender
{
    class CellRenderImp;
}

namespace MWMechanics
{
    struct CreatureStats;
    struct NpcStats;
    struct Movement;
}

namespace MWWorld
{
    class Ptr;
    class Environment;
    class ContainerStore;
    class InventoryStore;

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

            virtual void insertObjectRendering (const Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const;
            virtual void insertObject(const Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const;
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

            virtual bool hasToolTip (const Ptr& ptr) const;
            ///< @return true if this object has a tooltip when focused (default implementation: false)

            virtual MWGui::ToolTipInfo getToolTipInfo (const Ptr& ptr) const;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

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

            virtual ContainerStore& getContainerStore (const Ptr& ptr) const;
            ///< Return container store or throw an exception, if class does not have a
            /// container store (default implementation: throw an exceoption)

            virtual InventoryStore& getInventoryStore (const Ptr& ptr) const;
            ///< Return inventory store or throw an exception, if class does not have a
            /// inventory store (default implementation: throw an exceoption)

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

            virtual MWMechanics::Movement& getMovementSettings (const Ptr& ptr) const;
            ///< Return desired movement.

            virtual Ogre::Vector3 getMovementVector (const Ptr& ptr) const;
            ///< Return desired movement vector (determined based on movement settings,
            /// stance and stats).

            virtual std::pair<std::vector<int>, bool> getEquipmentSlots (const Ptr& ptr) const;
            ///< \return first: Return IDs of the slot this object can be equipped in; second: can object
            /// stay stacked when equipped?
            ///
            /// Default implementation: return (empty vector, false).

            virtual int getEquipmentSkill (const Ptr& ptr, const Environment& environment)
                const;
            /// Return the index of the skill this item corresponds to when equiopped or -1, if there is
            /// no such skill.
            /// (default implementation: return -1)

            virtual int getValue (const Ptr& ptr) const;
            ///< Return trade value of the object. Throws an exception, if the object can't be traded.
            /// (default implementation: throws an exception)

            static const Class& get (const std::string& key);
            ///< If there is no class for this \a key, an exception is thrown.

            static const Class& get (const Ptr& ptr);
            ///< If there is no class for this pointer, an exception is thrown.

            static void registerClass (const std::string& key,  boost::shared_ptr<Class> instance);

            virtual std::string getUpSoundId (const Ptr& ptr, const MWWorld::Environment& environment) const;
            ///< Return the up sound ID of \a ptr or throw an exception, if class does not support ID retrieval
            /// (default implementation: throw an exception)

            virtual std::string getDownSoundId (const Ptr& ptr, const MWWorld::Environment& environment) const;
            ///< Return the down sound ID of \a ptr or throw an exception, if class does not support ID retrieval
            /// (default implementation: throw an exception)
    };
}

#endif
