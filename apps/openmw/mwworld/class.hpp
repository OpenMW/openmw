#ifndef GAME_MWWORLD_CLASS_H
#define GAME_MWWORLD_CLASS_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <osg/Vec4f>

#include "doorstate.hpp"
#include "ptr.hpp"

#include "../mwmechanics/aisetting.hpp"
#include "../mwmechanics/damagesourcetype.hpp"

#include <components/esm/refid.hpp>
#include <components/vfs/pathutil.hpp>

namespace osg
{
    class Quat;
}

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
    class NpcStats;
    struct Movement;
    class CreatureStats;
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
        const unsigned mType;

        static std::map<unsigned, Class*>& getClasses();

    protected:
        explicit Class(unsigned type)
            : mType(type)
        {
        }

        std::unique_ptr<Action> defaultItemActivate(const Ptr& ptr, const Ptr& actor) const;
        ///< Generate default action for activating inventory items

        virtual Ptr copyToCellImpl(const ConstPtr& ptr, CellStore& cell) const;

    public:
        virtual ~Class() = default;
        Class(const Class&) = delete;
        Class& operator=(const Class&) = delete;

        unsigned int getType() const { return mType; }

        virtual void insertObjectRendering(
            const Ptr& ptr, const std::string& mesh, MWRender::RenderingInterface& renderingInterface) const;
        virtual void insertObject(const Ptr& ptr, const std::string& mesh, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const;
        ///< Add reference into a cell for rendering (default implementation: don't render anything).
        virtual void insertObjectPhysics(const Ptr& ptr, const std::string& mesh, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const;

        virtual std::string_view getName(const ConstPtr& ptr) const = 0;
        ///< \return name or ID; can return an empty string.

        virtual void adjustPosition(const MWWorld::Ptr& ptr, bool force) const;
        ///< Adjust position to stand on ground. Must be called post model load
        /// @param force do this even if the ptr is flying

        virtual MWMechanics::CreatureStats& getCreatureStats(const Ptr& ptr) const;
        ///< Return creature stats or throw an exception, if class does not have creature stats
        /// (default implementation: throw an exception)

        virtual bool hasToolTip(const ConstPtr& ptr) const;
        ///< @return true if this object has a tooltip when focused (default implementation: true)

        virtual MWGui::ToolTipInfo getToolTipInfo(const ConstPtr& ptr, int count) const;
        ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

        virtual bool showsInInventory(const ConstPtr& ptr) const;
        ///< Return whether ptr shows in inventory views.
        /// Hidden items are not displayed and cannot be (re)moved by the user.
        /// \return True if shown, false if hidden.

        virtual MWMechanics::NpcStats& getNpcStats(const Ptr& ptr) const;
        ///< Return NPC stats or throw an exception, if class does not have NPC stats
        /// (default implementation: throw an exception)

        virtual bool hasItemHealth(const ConstPtr& ptr) const;
        ///< \return Item health data available? (default implementation: false)

        virtual int getItemHealth(const ConstPtr& ptr) const;
        ///< Return current item health or throw an exception if class does not have item health

        virtual float getItemNormalizedHealth(const ConstPtr& ptr) const;
        ///< Return current item health re-scaled to maximum health

        virtual int getItemMaxHealth(const ConstPtr& ptr) const;
        ///< Return item max health or throw an exception, if class does not have item health
        /// (default implementation: throw an exception)

        virtual bool evaluateHit(const Ptr& ptr, Ptr& victim, osg::Vec3f& hitPosition) const;
        ///< Evaluate the victim of a melee hit produced by ptr in the current circumstances and return dice roll
        ///< success.
        /// (default implementation: throw an exception)

        virtual void hit(const Ptr& ptr, float attackStrength, int type = -1, const Ptr& victim = Ptr(),
            const osg::Vec3f& hitPosition = osg::Vec3f(), bool success = false) const;
        ///< Execute a melee hit on the victim at hitPosition, using the current weapon. If the hit was successful,
        ///< apply damage and process corresponding events.
        /// \param attackStrength how long the attack was charged for, a value in 0-1 range.
        /// \param type - type of attack, one of the MWMechanics::CreatureStats::AttackType
        ///               enums. ignored for creature attacks.
        /// (default implementation: throw an exception)

        virtual void onHit(const MWWorld::Ptr& ptr, const std::map<std::string, float>& damages, ESM::RefId object,
            const MWWorld::Ptr& attacker, bool successful, const MWMechanics::DamageSourceType sourceType) const;
        ///< Alerts \a ptr that it's being hit for \a damages by \a object (sword, arrow, etc). \a attacker specifies
        ///< the
        /// actor responsible for the attack. \a successful specifies if the hit is
        /// successful or not. \a sourceType classifies the damage source.

        virtual std::unique_ptr<Action> activate(const Ptr& ptr, const Ptr& actor) const;
        ///< Generate action for activation (default implementation: return a null action).

        virtual std::unique_ptr<Action> use(const Ptr& ptr, bool force = false) const;
        ///< Generate action for using via inventory menu (default implementation: return a
        /// null action).

        virtual ContainerStore& getContainerStore(const Ptr& ptr) const;
        ///< Return container store or throw an exception, if class does not have a
        /// container store (default implementation: throw an exception)

        virtual InventoryStore& getInventoryStore(const Ptr& ptr) const;
        ///< Return inventory store or throw an exception, if class does not have a
        /// inventory store (default implementation: throw an exception)

        virtual bool hasInventoryStore(const ConstPtr& ptr) const;
        ///< Does this object have an inventory store, i.e. equipment slots? (default implementation: false)

        virtual bool canLock(const ConstPtr& ptr) const;

        virtual void setRemainingUsageTime(const Ptr& ptr, float duration) const;
        ///< Sets the remaining duration of the object, such as an equippable light
        /// source. (default implementation: throw an exception)

        virtual float getRemainingUsageTime(const ConstPtr& ptr) const;
        ///< Returns the remaining duration of the object, such as an equippable light
        /// source. (default implementation: -1, i.e. infinite)

        virtual ESM::RefId getScript(const ConstPtr& ptr) const;
        ///< Return name of the script attached to ptr (default implementation: return an empty
        /// string).

        virtual float getWalkSpeed(const Ptr& ptr) const;
        virtual float getRunSpeed(const Ptr& ptr) const;
        virtual float getSwimSpeed(const Ptr& ptr) const;

        /// Return maximal movement speed for the current state.
        virtual float getMaxSpeed(const Ptr& ptr) const;

        /// Return current movement speed.
        virtual float getCurrentSpeed(const Ptr& ptr) const;

        virtual float getJump(const MWWorld::Ptr& ptr) const;
        ///< Return jump velocity (not accounting for movement)

        virtual MWMechanics::Movement& getMovementSettings(const Ptr& ptr) const;
        ///< Return desired movement.

        virtual osg::Vec3f getRotationVector(const Ptr& ptr) const;
        ///< Return desired rotations, as euler angles. Sets getMovementSettings(ptr).mRotation to zero.

        virtual std::pair<std::vector<int>, bool> getEquipmentSlots(const ConstPtr& ptr) const;
        ///< \return first: Return IDs of the slot this object can be equipped in; second: can object
        /// stay stacked when equipped?
        ///
        /// Default implementation: return (empty vector, false).

        virtual ESM::RefId getEquipmentSkill(const ConstPtr& ptr, bool useLuaInterfaceIfAvailable = false) const;
        /// Return the index of the skill this item corresponds to when equipped.
        /// (default implementation: return empty ref id)

        virtual int getValue(const ConstPtr& ptr) const;
        ///< Return trade value of the object. Throws an exception, if the object can't be traded.
        /// (default implementation: throws an exception)

        virtual float getCapacity(const MWWorld::Ptr& ptr) const;
        ///< Return total weight that fits into the object. Throws an exception, if the object can't
        /// hold other objects.
        /// (default implementation: throws an exception)

        virtual float getEncumbrance(const MWWorld::Ptr& ptr) const;
        ///< Returns total weight of objects inside this object (including modifications from magic
        /// effects). Throws an exception, if the object can't hold other objects.
        /// (default implementation: throws an exception)

        virtual float getNormalizedEncumbrance(const MWWorld::Ptr& ptr) const;
        ///< Returns encumbrance re-scaled to capacity

        virtual bool consume(const MWWorld::Ptr& consumable, const MWWorld::Ptr& actor) const;
        ///< Consume an item, e. g. a potion.

        virtual void skillUsageSucceeded(
            const MWWorld::Ptr& ptr, ESM::RefId skill, int usageType, float extraFactor = 1.f) const;
        ///< Inform actor \a ptr that a skill use has succeeded.
        ///
        /// (default implementations: throws an exception)

        virtual bool isEssential(const MWWorld::ConstPtr& ptr) const;
        ///< Is \a ptr essential? (i.e. may losing \a ptr make the game unwinnable)
        ///
        /// (default implementation: return false)

        virtual const ESM::RefId& getUpSoundId(const ConstPtr& ptr) const;
        ///< Return the up sound ID of \a ptr or throw an exception, if class does not support ID retrieval
        /// (default implementation: throw an exception)

        virtual const ESM::RefId& getDownSoundId(const ConstPtr& ptr) const;
        ///< Return the down sound ID of \a ptr or throw an exception, if class does not support ID retrieval
        /// (default implementation: throw an exception)

        virtual ESM::RefId getSoundIdFromSndGen(const Ptr& ptr, std::string_view type) const;
        ///< Returns the sound ID for \a ptr of the given soundgen \a type.

        virtual float getArmorRating(const MWWorld::Ptr& ptr, bool useLuaInterfaceIfAvailable = false) const;
        ///< @return combined armor rating of this actor

        virtual const std::string& getInventoryIcon(const MWWorld::ConstPtr& ptr) const;
        ///< Return name of inventory icon.

        virtual ESM::RefId getEnchantment(const MWWorld::ConstPtr& ptr) const;
        ///< @return the enchantment ID if the object is enchanted, otherwise an empty string
        /// (default implementation: return empty string)

        virtual int getEnchantmentPoints(const MWWorld::ConstPtr& ptr) const;
        ///< @return the number of enchantment points available for possible enchanting

        virtual void adjustScale(const MWWorld::ConstPtr& ptr, osg::Vec3f& scale, bool rendering) const;
        /// @param rendering Indicates if the scale to adjust is for the rendering mesh, or for the collision mesh

        virtual bool canSell(const MWWorld::ConstPtr& item, int npcServices) const;
        ///< Determine whether or not \a item can be sold to an npc with the given \a npcServices

        virtual int getServices(const MWWorld::ConstPtr& actor) const;

        virtual std::string_view getModel(const MWWorld::ConstPtr& ptr) const;

        virtual VFS::Path::Normalized getCorrectedModel(const MWWorld::ConstPtr& ptr) const;

        virtual bool useAnim() const;
        ///< Whether or not to use animated variant of model (default false)

        virtual void getModelsToPreload(const MWWorld::ConstPtr& ptr, std::vector<std::string_view>& models) const;
        ///< Get a list of models to preload that this object may use (directly or indirectly). default implementation:
        ///< list getModel().

        virtual const ESM::RefId& applyEnchantment(
            const MWWorld::ConstPtr& ptr, const ESM::RefId& enchId, int enchCharge, const std::string& newName) const;
        ///< Creates a new record using \a ptr as template, with the given name and the given enchantment applied to it.

        virtual std::pair<int, std::string_view> canBeEquipped(
            const MWWorld::ConstPtr& ptr, const MWWorld::Ptr& npc) const;
        ///< Return 0 if player cannot equip item. 1 if can equip. 2 if it's twohanded weapon. 3 if twohanded weapon
        ///< conflicts with that.
        ///  Second item in the pair specifies the error message

        virtual float getWeight(const MWWorld::ConstPtr& ptr) const;

        virtual bool isPersistent(const MWWorld::ConstPtr& ptr) const;

        virtual bool isKey(const MWWorld::ConstPtr& ptr) const { return false; }

        virtual bool isGold(const MWWorld::ConstPtr& ptr) const { return false; }

        virtual bool isSoulGem(const MWWorld::ConstPtr& ptr) const { return false; }

        virtual bool allowTelekinesis(const MWWorld::ConstPtr& ptr) const { return true; }
        ///< Return whether this class of object can be activated with telekinesis

        virtual Ptr copyToCell(const ConstPtr& ptr, CellStore& cell, int count) const;

        // Similar to `copyToCell`, but preserves RefNum and moves LuaScripts.
        // The original is expected to be removed after calling this function,
        // but this function itself doesn't remove the original.
        virtual Ptr moveToCell(const Ptr& ptr, CellStore& cell) const;
        Ptr moveToCell(const Ptr& ptr, CellStore& cell, const ESM::Position& pos) const;

        Ptr copyToCell(const ConstPtr& ptr, CellStore& cell, const ESM::Position& pos, int count) const;

        virtual bool isActivator() const { return false; }

        virtual bool isActor() const { return false; }

        virtual bool isNpc() const { return false; }

        virtual bool isDoor() const { return false; }

        // True if it is an item that can be picked up.
        virtual bool isItem(const MWWorld::ConstPtr& ptr) const { return false; }

        virtual bool isBipedal(const MWWorld::ConstPtr& ptr) const;
        virtual bool canFly(const MWWorld::ConstPtr& ptr) const;
        virtual bool canSwim(const MWWorld::ConstPtr& ptr) const;
        virtual bool canWalk(const MWWorld::ConstPtr& ptr) const;
        bool isPureWaterCreature(const MWWorld::ConstPtr& ptr) const;
        bool isPureFlyingCreature(const MWWorld::ConstPtr& ptr) const;
        bool isPureLandCreature(const MWWorld::Ptr& ptr) const;
        bool isMobile(const MWWorld::Ptr& ptr) const;

        virtual float getSkill(const MWWorld::Ptr& ptr, ESM::RefId id) const;

        virtual void readAdditionalState(const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const;
        ///< Read additional state from \a state into \a ptr.

        virtual void writeAdditionalState(const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const;
        ///< Write additional state from \a ptr into \a state.

        static const Class& get(unsigned int key);
        ///< If there is no class for this \a key, an exception is thrown.

        static void registerClass(Class& instance);

        virtual int getBaseGold(const MWWorld::ConstPtr& ptr) const;

        virtual bool isClass(const MWWorld::ConstPtr& ptr, std::string_view className) const;

        virtual DoorState getDoorState(const MWWorld::ConstPtr& ptr) const;
        /// This does not actually cause the door to move. Use World::activateDoor instead.
        virtual void setDoorState(const MWWorld::Ptr& ptr, DoorState state) const;

        virtual void respawn(const MWWorld::Ptr& ptr) const {}

        /// Returns sound id
        virtual ESM::RefId getSound(const MWWorld::ConstPtr& ptr) const;

        virtual int getBaseFightRating(const MWWorld::ConstPtr& ptr) const;

        virtual ESM::RefId getPrimaryFaction(const MWWorld::ConstPtr& ptr) const;
        virtual int getPrimaryFactionRank(const MWWorld::ConstPtr& ptr) const;

        /// Get the effective armor rating, factoring in the actor's skills, for the given armor.
        virtual float getSkillAdjustedArmorRating(
            const MWWorld::ConstPtr& armor, const MWWorld::Ptr& actor, bool useLuaInterfaceIfAvailable = false) const;

        virtual osg::Vec4f getEnchantmentColor(const MWWorld::ConstPtr& item) const;

        virtual void setBaseAISetting(const ESM::RefId& id, MWMechanics::AiSetting setting, int value) const;

        virtual void modifyBaseInventory(const ESM::RefId& actorId, const ESM::RefId& itemId, int amount) const;
    };
}

#endif
