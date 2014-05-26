#ifndef GAME_MWWORLD_CLASS_H
#define GAME_MWWORLD_CLASS_H

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "ptr.hpp"

namespace ESM
{
    struct ObjectState;
}

namespace Ogre
{
    class Vector3;
}

namespace MWRender
{
    class RenderingInterface;
}

namespace MWMechanics
{
    class CreatureStats;
    class NpcStats;
    struct Movement;
}

namespace MWGui
{
    struct ToolTipInfo;
}

namespace ESM
{
    struct Position;
}

namespace MWWorld
{
    class Ptr;
    class ContainerStore;
    class InventoryStore;
    class PhysicsSystem;
    class CellStore;
    class Action;

    /// \brief Base class for referenceable esm records
    class Class
    {
            static std::map<std::string, boost::shared_ptr<Class> > sClasses;

            std::string mTypeName;

            // not implemented
            Class (const Class&);
            Class& operator= (const Class&);

        protected:

            Class();

            boost::shared_ptr<Action> defaultItemActivate(const Ptr &ptr, const Ptr &actor) const;
            ///< Generate default action for activating inventory items

            virtual Ptr copyToCellImpl(const Ptr &ptr, CellStore &cell) const;

        public:

            /// NPC-stances.
            enum Stance
            {
                Run, Sneak
            };

            virtual ~Class();

            const std::string& getTypeName() const {
                return mTypeName;
            }

            virtual std::string getId (const Ptr& ptr) const;
            ///< Return ID of \a ptr or throw an exception, if class does not support ID retrieval
            /// (default implementation: throw an exception)

            virtual void insertObjectRendering (const Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const;
            virtual void insertObject(const Ptr& ptr, MWWorld::PhysicsSystem& physics) const;
            ///< Add reference into a cell for rendering (default implementation: don't render anything).

            virtual std::string getName (const Ptr& ptr) const = 0;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual void adjustPosition(const MWWorld::Ptr& ptr) const;
            ///< Adjust position to stand on ground. Must be called post model load

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

            virtual int getItemHealth (const Ptr& ptr) const;
            ///< Return current item health or throw an exception if class does not have item health

            virtual int getItemMaxHealth (const Ptr& ptr) const;
            ///< Return item max health or throw an exception, if class does not have item health
            /// (default implementation: throw an exception)

            virtual void hit(const Ptr& ptr, int type=-1) const;
            ///< Execute a melee hit, using the current weapon. This will check the relevant skills
            /// of the given attacker, and whoever is hit.
            /// \param type - type of attack, one of the MWMechanics::CreatureStats::AttackType
            ///               enums. ignored for creature attacks.
            /// (default implementation: throw an exceoption)

            virtual void onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, bool successful) const;
            ///< Alerts \a ptr that it's being hit for \a damage points to health if \a ishealth is
            /// true (else fatigue) by \a object (sword, arrow, etc). \a attacker specifies the
            /// actor responsible for the attack, and \a successful specifies if the hit is
            /// successful or not.

            virtual void block (const Ptr& ptr) const;
            ///< Play the appropriate sound for a blocked attack, depending on the currently equipped shield
            /// (default implementation: throw an exception)

            virtual void setActorHealth(const Ptr& ptr, float health, const Ptr& attacker=Ptr()) const;
            ///< Sets a new current health value for the actor, optionally specifying the object causing
            /// the change. Use this instead of using CreatureStats directly as this will make sure the
            /// correct dialog and actor states are properly handled when being hurt or healed.
            /// (default implementation: throw an exceoption)

            virtual boost::shared_ptr<Action> activate (const Ptr& ptr, const Ptr& actor) const;
            ///< Generate action for activation (default implementation: return a null action).

            virtual boost::shared_ptr<Action> use (const Ptr& ptr)
                const;
            ///< Generate action for using via inventory menu (default implementation: return a
            /// null action).

            virtual ContainerStore& getContainerStore (const Ptr& ptr) const;
            ///< Return container store or throw an exception, if class does not have a
            /// container store (default implementation: throw an exceoption)

            virtual InventoryStore& getInventoryStore (const Ptr& ptr) const;
            ///< Return inventory store or throw an exception, if class does not have a
            /// inventory store (default implementation: throw an exceoption)

            virtual bool hasInventoryStore (const Ptr& ptr) const;
            ///< Does this object have an inventory store, i.e. equipment slots? (default implementation: false)

            virtual void lock (const Ptr& ptr, int lockLevel = 0) const;
            ///< Lock object (default implementation: throw an exception)

            virtual void unlock (const Ptr& ptr) const;
            ///< Unlock object (default implementation: throw an exception)

            virtual void setRemainingUsageTime (const Ptr& ptr, float duration) const;
            ///< Sets the remaining duration of the object, such as an equippable light
            /// source. (default implementation: throw an exception)

            virtual float getRemainingUsageTime (const Ptr& ptr) const;
            ///< Returns the remaining duration of the object, such as an equippable light
            /// source. (default implementation: -1, i.e. infinite)

            virtual std::string getScript (const Ptr& ptr) const;
            ///< Return name of the script attached to ptr (default implementation: return an empty
            /// string).

            virtual float getSpeed (const Ptr& ptr) const;
            ///< Return movement speed.

            virtual float getJump(const MWWorld::Ptr &ptr) const;
            ///< Return jump velocity (not accounting for movement)

            virtual float getFallDamage(const MWWorld::Ptr &ptr, float fallHeight) const;
            ///< Return amount of health points lost when falling

            virtual MWMechanics::Movement& getMovementSettings (const Ptr& ptr) const;
            ///< Return desired movement.

            virtual Ogre::Vector3 getMovementVector (const Ptr& ptr) const;
            ///< Return desired movement vector (determined based on movement settings,
            /// stance and stats).

            virtual Ogre::Vector3 getRotationVector (const Ptr& ptr) const;
            ///< Return desired rotations, as euler angles.

            virtual std::pair<std::vector<int>, bool> getEquipmentSlots (const Ptr& ptr) const;
            ///< \return first: Return IDs of the slot this object can be equipped in; second: can object
            /// stay stacked when equipped?
            ///
            /// Default implementation: return (empty vector, false).

            virtual int getEquipmentSkill (const Ptr& ptr)
                const;
            /// Return the index of the skill this item corresponds to when equiopped or -1, if there is
            /// no such skill.
            /// (default implementation: return -1)

            virtual int getValue (const Ptr& ptr) const;
            ///< Return trade value of the object. Throws an exception, if the object can't be traded.
            /// (default implementation: throws an exception)

            virtual float getCapacity (const MWWorld::Ptr& ptr) const;
            ///< Return total weight that fits into the object. Throws an exception, if the object can't
            /// hold other objects.
            /// (default implementation: throws an exception)

            virtual float getEncumbrance (const MWWorld::Ptr& ptr) const;
            ///< Returns total weight of objects inside this object (including modifications from magic
            /// effects). Throws an exception, if the object can't hold other objects.
            /// (default implementation: throws an exception)

            virtual bool apply (const MWWorld::Ptr& ptr, const std::string& id,
                const MWWorld::Ptr& actor) const;
            ///< Apply \a id on \a ptr.
            /// \param actor Actor that is resposible for the ID being applied to \a ptr.
            /// \return Any effect?
            ///
            /// (default implementation: ignore and return false)

            virtual void skillUsageSucceeded (const MWWorld::Ptr& ptr, int skill, int usageType) const;
            ///< Inform actor \a ptr that a skill use has succeeded.
            ///
            /// (default implementations: throws an exception)

            virtual bool isEssential (const MWWorld::Ptr& ptr) const;
            ///< Is \a ptr essential? (i.e. may losing \a ptr make the game unwinnable)
            ///
            /// (default implementation: return false)

            virtual std::string getUpSoundId (const Ptr& ptr) const;
            ///< Return the up sound ID of \a ptr or throw an exception, if class does not support ID retrieval
            /// (default implementation: throw an exception)

            virtual std::string getDownSoundId (const Ptr& ptr) const;
            ///< Return the down sound ID of \a ptr or throw an exception, if class does not support ID retrieval
            /// (default implementation: throw an exception)

            virtual std::string getSoundIdFromSndGen(const Ptr &ptr, const std::string &type) const;
            ///< Returns the sound ID for \a ptr of the given soundgen \a type.

            virtual float getArmorRating (const MWWorld::Ptr& ptr) const;
            ///< @return combined armor rating of this actor

            virtual std::string getInventoryIcon (const MWWorld::Ptr& ptr) const;
            ///< Return name of inventory icon.

            virtual std::string getEnchantment (const MWWorld::Ptr& ptr) const;
            ///< @return the enchantment ID if the object is enchanted, otherwise an empty string
            /// (default implementation: return empty string)

            virtual int getEnchantmentPoints (const MWWorld::Ptr& ptr) const;
            ///< @return the number of enchantment points available for possible enchanting

            virtual void adjustScale(const MWWorld::Ptr& ptr,float& scale) const;

            virtual void adjustRotation(const MWWorld::Ptr& ptr,float& x,float& y,float& z) const;

            virtual bool canSell (const MWWorld::Ptr& item, int npcServices) const;
            ///< Determine whether or not \a item can be sold to an npc with the given \a npcServices

            virtual int getServices (const MWWorld::Ptr& actor) const;

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;

            virtual std::string applyEnchantment(const MWWorld::Ptr &ptr, const std::string& enchId, int enchCharge, const std::string& newName) const;
            ///< Creates a new record using \a ptr as template, with the given name and the given enchantment applied to it.

            virtual std::pair<int, std::string> canBeEquipped(const MWWorld::Ptr &ptr, const MWWorld::Ptr &npc) const;
            ///< Return 0 if player cannot equip item. 1 if can equip. 2 if it's twohanded weapon. 3 if twohanded weapon conflicts with that.
            ///  Second item in the pair specifies the error message

            virtual float getWeight (const MWWorld::Ptr& ptr) const;

            virtual bool isPersistent (const MWWorld::Ptr& ptr) const;

            virtual bool isKey (const MWWorld::Ptr& ptr) const { return false; }

            /// Get a blood texture suitable for \a ptr (see Blood Texture 0-2 in Morrowind.ini)
            virtual int getBloodTexture (const MWWorld::Ptr& ptr) const;

            virtual Ptr
            copyToCell(const Ptr &ptr, CellStore &cell) const;

            virtual Ptr
            copyToCell(const Ptr &ptr, CellStore &cell, const ESM::Position &pos) const;

            virtual bool isActor() const {
                return false;
            }

            virtual bool isNpc() const {
                return false;
            }

            virtual bool isBipedal(const MWWorld::Ptr& ptr) const;
            virtual bool canFly(const MWWorld::Ptr& ptr) const;
            virtual bool canSwim(const MWWorld::Ptr& ptr) const;
            virtual bool canWalk(const MWWorld::Ptr& ptr) const;

            virtual int getSkill(const MWWorld::Ptr& ptr, int skill) const;

            virtual void readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
                const;
            ///< Read additional state from \a state into \a ptr.

            virtual void writeAdditionalState (const MWWorld::Ptr& ptr, ESM::ObjectState& state)
                const;
            ///< Write additional state from \a ptr into \a state.

            static const Class& get (const std::string& key);
            ///< If there is no class for this \a key, an exception is thrown.

            static void registerClass (const std::string& key,  boost::shared_ptr<Class> instance);

            virtual int getBaseGold(const MWWorld::Ptr& ptr) const;

            virtual bool isClass(const MWWorld::Ptr& ptr, const std::string &className) const;

            /// 0 = nothing, 1 = opening, 2 = closing
            virtual int getDoorState (const MWWorld::Ptr &ptr) const;
            /// This does not actually cause the door to move. Use World::activateDoor instead.
            virtual void setDoorState (const MWWorld::Ptr &ptr, int state) const;

            virtual void respawn (const MWWorld::Ptr& ptr) const {}

            virtual void restock (const MWWorld::Ptr& ptr) const {}
    };
}

#endif
