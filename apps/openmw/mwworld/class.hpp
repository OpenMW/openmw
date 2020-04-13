#ifndef GAME_MWWORLD_CLASS_H
#define GAME_MWWORLD_CLASS_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <osg/Vec4f>

#include "ptr.hpp"
#include "doorstate.hpp"

namespace ESM
{
    struct ObjectState;
}

namespace MWRender
{
    class RenderingInterface;
}

namespace MWPhysics
{
    class PhysicsSystem;
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
    class ContainerStore;
    class InventoryStore;
    class CellStore;
    class Action;

    /// \brief Base class for referenceable esm records
    class Class
    {
            static std::map<std::string, std::shared_ptr<Class> > sClasses;

            std::string mTypeName;

            // not implemented
            Class (const Class&);
            Class& operator= (const Class&);

        protected:

            Class();

            std::shared_ptr<Action> defaultItemActivate(const Ptr &ptr, const Ptr &actor) const;
            ///< Generate default action for activating inventory items

            virtual Ptr copyToCellImpl(const ConstPtr &ptr, CellStore &cell) const;

        public:

            virtual ~Class();

            const std::string& getTypeName() const {
                return mTypeName;
            }

            virtual void insertObjectRendering (const Ptr& ptr, const std::string& mesh, MWRender::RenderingInterface& renderingInterface) const;
            virtual void insertObject(const Ptr& ptr, const std::string& mesh, MWPhysics::PhysicsSystem& physics) const;
            ///< Add reference into a cell for rendering (default implementation: don't render anything).

            virtual std::string getName (const ConstPtr& ptr) const = 0;
            ///< \return name or ID; can return an empty string.

            virtual void adjustPosition(const MWWorld::Ptr& ptr, bool force) const;
            ///< Adjust position to stand on ground. Must be called post model load
            /// @param force do this even if the ptr is flying

            virtual MWMechanics::CreatureStats& getCreatureStats (const Ptr& ptr) const;
            ///< Return creature stats or throw an exception, if class does not have creature stats
            /// (default implementation: throw an exception)

            virtual bool hasToolTip (const ConstPtr& ptr) const;
            ///< @return true if this object has a tooltip when focused (default implementation: true)

            virtual MWGui::ToolTipInfo getToolTipInfo (const ConstPtr& ptr, int count) const;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            virtual bool showsInInventory (const ConstPtr& ptr) const;
            ///< Return whether ptr shows in inventory views.
            /// Hidden items are not displayed and cannot be (re)moved by the user.
            /// \return True if shown, false if hidden.

            virtual MWMechanics::NpcStats& getNpcStats (const Ptr& ptr) const;
            ///< Return NPC stats or throw an exception, if class does not have NPC stats
            /// (default implementation: throw an exception)

            virtual bool hasItemHealth (const ConstPtr& ptr) const;
            ///< \return Item health data available? (default implementation: false)

            virtual int getItemHealth (const ConstPtr& ptr) const;
            ///< Return current item health or throw an exception if class does not have item health

            virtual float getItemNormalizedHealth (const ConstPtr& ptr) const;
            ///< Return current item health re-scaled to maximum health

            virtual int getItemMaxHealth (const ConstPtr& ptr) const;
            ///< Return item max health or throw an exception, if class does not have item health
            /// (default implementation: throw an exception)

            virtual void hit(const Ptr& ptr, float attackStrength, int type=-1) const;
            ///< Execute a melee hit, using the current weapon. This will check the relevant skills
            /// of the given attacker, and whoever is hit.
            /// \param attackStrength how long the attack was charged for, a value in 0-1 range.
            /// \param type - type of attack, one of the MWMechanics::CreatureStats::AttackType
            ///               enums. ignored for creature attacks.
            /// (default implementation: throw an exception)

            virtual void onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, const osg::Vec3f &hitPosition, bool successful) const;
            ///< Alerts \a ptr that it's being hit for \a damage points to health if \a ishealth is
            /// true (else fatigue) by \a object (sword, arrow, etc). \a attacker specifies the
            /// actor responsible for the attack, and \a successful specifies if the hit is
            /// successful or not.

            virtual void block (const Ptr& ptr) const;
            ///< Play the appropriate sound for a blocked attack, depending on the currently equipped shield
            /// (default implementation: throw an exception)

            virtual std::shared_ptr<Action> activate (const Ptr& ptr, const Ptr& actor) const;
            ///< Generate action for activation (default implementation: return a null action).

            virtual std::shared_ptr<Action> use (const Ptr& ptr, bool force=false)
                const;
            ///< Generate action for using via inventory menu (default implementation: return a
            /// null action).

            virtual ContainerStore& getContainerStore (const Ptr& ptr) const;
            ///< Return container store or throw an exception, if class does not have a
            /// container store (default implementation: throw an exception)

            virtual InventoryStore& getInventoryStore (const Ptr& ptr) const;
            ///< Return inventory store or throw an exception, if class does not have a
            /// inventory store (default implementation: throw an exception)

            virtual bool hasInventoryStore (const Ptr& ptr) const;
            ///< Does this object have an inventory store, i.e. equipment slots? (default implementation: false)

            virtual bool canLock (const ConstPtr& ptr) const;

            virtual void setRemainingUsageTime (const Ptr& ptr, float duration) const;
            ///< Sets the remaining duration of the object, such as an equippable light
            /// source. (default implementation: throw an exception)

            virtual float getRemainingUsageTime (const ConstPtr& ptr) const;
            ///< Returns the remaining duration of the object, such as an equippable light
            /// source. (default implementation: -1, i.e. infinite)

            virtual std::string getScript (const ConstPtr& ptr) const;
            ///< Return name of the script attached to ptr (default implementation: return an empty
            /// string).

            virtual float getSpeed (const Ptr& ptr) const;
            ///< Return movement speed.

            virtual float getJump(const MWWorld::Ptr &ptr) const;
            ///< Return jump velocity (not accounting for movement)

            virtual MWMechanics::Movement& getMovementSettings (const Ptr& ptr) const;
            ///< Return desired movement.

            virtual osg::Vec3f getRotationVector (const Ptr& ptr) const;
            ///< Return desired rotations, as euler angles.

            virtual std::pair<std::vector<int>, bool> getEquipmentSlots (const ConstPtr& ptr) const;
            ///< \return first: Return IDs of the slot this object can be equipped in; second: can object
            /// stay stacked when equipped?
            ///
            /// Default implementation: return (empty vector, false).

            virtual int getEquipmentSkill (const ConstPtr& ptr)
                const;
            /// Return the index of the skill this item corresponds to when equipped or -1, if there is
            /// no such skill.
            /// (default implementation: return -1)

            virtual int getValue (const ConstPtr& ptr) const;
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

            virtual float getNormalizedEncumbrance (const MWWorld::Ptr& ptr) const;
            ///< Returns encumbrance re-scaled to capacity

            virtual bool apply (const MWWorld::Ptr& ptr, const std::string& id,
                const MWWorld::Ptr& actor) const;
            ///< Apply \a id on \a ptr.
            /// \param actor Actor that is resposible for the ID being applied to \a ptr.
            /// \return Any effect?
            ///
            /// (default implementation: ignore and return false)

            virtual void skillUsageSucceeded (const MWWorld::Ptr& ptr, int skill, int usageType, float extraFactor=1.f) const;
            ///< Inform actor \a ptr that a skill use has succeeded.
            ///
            /// (default implementations: throws an exception)

            virtual bool isEssential (const MWWorld::ConstPtr& ptr) const;
            ///< Is \a ptr essential? (i.e. may losing \a ptr make the game unwinnable)
            ///
            /// (default implementation: return false)

            virtual std::string getUpSoundId (const ConstPtr& ptr) const;
            ///< Return the up sound ID of \a ptr or throw an exception, if class does not support ID retrieval
            /// (default implementation: throw an exception)

            virtual std::string getDownSoundId (const ConstPtr& ptr) const;
            ///< Return the down sound ID of \a ptr or throw an exception, if class does not support ID retrieval
            /// (default implementation: throw an exception)

            virtual std::string getSoundIdFromSndGen(const Ptr &ptr, const std::string &type) const;
            ///< Returns the sound ID for \a ptr of the given soundgen \a type.

            virtual float getArmorRating (const MWWorld::Ptr& ptr) const;
            ///< @return combined armor rating of this actor

            virtual std::string getInventoryIcon (const MWWorld::ConstPtr& ptr) const;
            ///< Return name of inventory icon.

            virtual std::string getEnchantment (const MWWorld::ConstPtr& ptr) const;
            ///< @return the enchantment ID if the object is enchanted, otherwise an empty string
            /// (default implementation: return empty string)

            virtual int getEnchantmentPoints (const MWWorld::ConstPtr& ptr) const;
            ///< @return the number of enchantment points available for possible enchanting

            virtual void adjustScale(const MWWorld::ConstPtr& ptr, osg::Vec3f& scale, bool rendering) const;
            /// @param rendering Indicates if the scale to adjust is for the rendering mesh, or for the collision mesh

            virtual bool canSell (const MWWorld::ConstPtr& item, int npcServices) const;
            ///< Determine whether or not \a item can be sold to an npc with the given \a npcServices

            virtual int getServices (const MWWorld::ConstPtr& actor) const;

            virtual std::string getModel(const MWWorld::ConstPtr &ptr) const;

            virtual bool useAnim() const;
            ///< Whether or not to use animated variant of model (default false)

            virtual void getModelsToPreload(const MWWorld::Ptr& ptr, std::vector<std::string>& models) const;
            ///< Get a list of models to preload that this object may use (directly or indirectly). default implementation: list getModel().

            virtual std::string applyEnchantment(const MWWorld::ConstPtr &ptr, const std::string& enchId, int enchCharge, const std::string& newName) const;
            ///< Creates a new record using \a ptr as template, with the given name and the given enchantment applied to it.

            virtual std::pair<int, std::string> canBeEquipped(const MWWorld::ConstPtr &ptr, const MWWorld::Ptr &npc) const;
            ///< Return 0 if player cannot equip item. 1 if can equip. 2 if it's twohanded weapon. 3 if twohanded weapon conflicts with that.
            ///  Second item in the pair specifies the error message

            virtual float getWeight (const MWWorld::ConstPtr& ptr) const;

            virtual bool isPersistent (const MWWorld::ConstPtr& ptr) const;

            virtual bool isKey (const MWWorld::ConstPtr& ptr) const { return false; }

            virtual bool isGold(const MWWorld::ConstPtr& ptr) const { return false; }

            virtual bool allowTelekinesis(const MWWorld::ConstPtr& ptr) const { return true; }
            ///< Return whether this class of object can be activated with telekinesis

            /// Get a blood texture suitable for \a ptr (see Blood Texture 0-2 in Morrowind.ini)
            virtual int getBloodTexture (const MWWorld::ConstPtr& ptr) const;

            virtual Ptr copyToCell(const ConstPtr &ptr, CellStore &cell, int count) const;

            virtual Ptr copyToCell(const ConstPtr &ptr, CellStore &cell, const ESM::Position &pos, int count) const;

            virtual bool isActivator() const {
                return false;
            }

            virtual bool isActor() const {
                return false;
            }

            virtual bool isNpc() const {
                return false;
            }

            virtual bool isDoor() const {
                return false;
            }

            virtual bool isBipedal(const MWWorld::ConstPtr& ptr) const;
            virtual bool canFly(const MWWorld::ConstPtr& ptr) const;
            virtual bool canSwim(const MWWorld::ConstPtr& ptr) const;
            virtual bool canWalk(const MWWorld::ConstPtr& ptr) const;
            bool isPureWaterCreature(const MWWorld::ConstPtr& ptr) const;
            bool isPureFlyingCreature(const MWWorld::ConstPtr& ptr) const;
            bool isPureLandCreature(const MWWorld::Ptr& ptr) const;
            bool isMobile(const MWWorld::Ptr& ptr) const;

            virtual int getSkill(const MWWorld::Ptr& ptr, int skill) const;

            virtual void readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
                const;
            ///< Read additional state from \a state into \a ptr.

            virtual void writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state)
                const;
            ///< Write additional state from \a ptr into \a state.

            static const Class& get (const std::string& key);
            ///< If there is no class for this \a key, an exception is thrown.

            static void registerClass (const std::string& key,  std::shared_ptr<Class> instance);

            virtual int getBaseGold(const MWWorld::ConstPtr& ptr) const;

            virtual bool isClass(const MWWorld::ConstPtr& ptr, const std::string &className) const;

            virtual DoorState getDoorState (const MWWorld::ConstPtr &ptr) const;
            /// This does not actually cause the door to move. Use World::activateDoor instead.
            virtual void setDoorState (const MWWorld::Ptr &ptr, DoorState state) const;

            virtual void respawn (const MWWorld::Ptr& ptr) const {}

            virtual void restock (const MWWorld::Ptr& ptr) const {}

            /// Returns sound id
            virtual std::string getSound(const MWWorld::ConstPtr& ptr) const;

            virtual int getBaseFightRating (const MWWorld::ConstPtr& ptr) const;

            virtual std::string getPrimaryFaction (const MWWorld::ConstPtr& ptr) const;
            virtual int getPrimaryFactionRank (const MWWorld::ConstPtr& ptr) const;

            /// Get the effective armor rating, factoring in the actor's skills, for the given armor.
            virtual float getEffectiveArmorRating(const MWWorld::ConstPtr& armor, const MWWorld::Ptr& actor) const;

            virtual osg::Vec4f getEnchantmentColor(const MWWorld::ConstPtr& item) const;
    };
}

#endif
