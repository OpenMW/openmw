#ifndef GAME_MWCLASS_NPC_H
#define GAME_MWCLASS_NPC_H

#include "../mwworld/class.hpp"

namespace ESM
{
    struct GameSetting;
}

namespace MWClass
{
    class Npc : public MWWorld::Class
    {
            void ensureCustomData (const MWWorld::Ptr& ptr) const;

            virtual MWWorld::Ptr
            copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const;

            struct GMST
            {
                const ESM::GameSetting *fMinWalkSpeed;
                const ESM::GameSetting *fMaxWalkSpeed;
                const ESM::GameSetting *fEncumberedMoveEffect;
                const ESM::GameSetting *fSneakSpeedMultiplier;
                const ESM::GameSetting *fAthleticsRunBonus;
                const ESM::GameSetting *fBaseRunMultiplier;
                const ESM::GameSetting *fMinFlySpeed;
                const ESM::GameSetting *fMaxFlySpeed;
                const ESM::GameSetting *fSwimRunBase;
                const ESM::GameSetting *fSwimRunAthleticsMult;
                const ESM::GameSetting *fJumpEncumbranceBase;
                const ESM::GameSetting *fJumpEncumbranceMultiplier;
                const ESM::GameSetting *fJumpAcrobaticsBase;
                const ESM::GameSetting *fJumpAcroMultiplier;
                const ESM::GameSetting *fJumpRunMultiplier;
                const ESM::GameSetting *fWereWolfRunMult;
                const ESM::GameSetting *fKnockDownMult;
                const ESM::GameSetting *iKnockDownOddsMult;
                const ESM::GameSetting *iKnockDownOddsBase;
                const ESM::GameSetting *fCombatArmorMinMult;
            };

            static const GMST& getGmst();

        public:

            virtual std::string getId (const MWWorld::Ptr& ptr) const;
            ///< Return ID of \a ptr

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const;

            virtual void adjustPosition(const MWWorld::Ptr& ptr, bool force) const;
            ///< Adjust position to stand on ground. Must be called post model load
            /// @param force do this even if the ptr is flying

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual MWMechanics::CreatureStats& getCreatureStats (const MWWorld::Ptr& ptr) const;
            ///< Return creature stats

            virtual MWMechanics::NpcStats& getNpcStats (const MWWorld::Ptr& ptr) const;
            ///< Return NPC stats

            virtual MWWorld::ContainerStore& getContainerStore (const MWWorld::Ptr& ptr) const;
            ///< Return container store

            virtual bool hasToolTip (const MWWorld::Ptr& ptr) const;
            ///< @return true if this object has a tooltip when focused (default implementation: false)

            virtual MWGui::ToolTipInfo getToolTipInfo (const MWWorld::Ptr& ptr) const;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            virtual MWWorld::InventoryStore& getInventoryStore (const MWWorld::Ptr& ptr) const;
            ///< Return inventory store

            virtual bool hasInventoryStore(const MWWorld::Ptr &ptr) const { return true; }

            virtual void hit(const MWWorld::Ptr& ptr, int type) const;

            virtual void onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, bool successful) const;

            virtual void block(const MWWorld::Ptr &ptr) const;

            virtual void setActorHealth(const MWWorld::Ptr& ptr, float health, const MWWorld::Ptr& attacker) const;

            virtual boost::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const;
            ///< Generate action for activation

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;
            ///< Return name of the script attached to ptr

            virtual float getSpeed (const MWWorld::Ptr& ptr) const;
            ///< Return movement speed.

            virtual float getJump(const MWWorld::Ptr &ptr) const;
            ///< Return jump velocity (not accounting for movement)

            virtual MWMechanics::Movement& getMovementSettings (const MWWorld::Ptr& ptr) const;
            ///< Return desired movement.

            virtual Ogre::Vector3 getMovementVector (const MWWorld::Ptr& ptr) const;
            ///< Return desired movement vector (determined based on movement settings,
            /// stance and stats).

            virtual Ogre::Vector3 getRotationVector (const MWWorld::Ptr& ptr) const;
            ///< Return desired rotations, as euler angles.

            virtual float getCapacity (const MWWorld::Ptr& ptr) const;
            ///< Return total weight that fits into the object. Throws an exception, if the object can't
            /// hold other objects.

            virtual float getEncumbrance (const MWWorld::Ptr& ptr) const;
            ///< Returns total weight of objects inside this object (including modifications from magic
            /// effects). Throws an exception, if the object can't hold other objects.

            virtual float getArmorRating (const MWWorld::Ptr& ptr) const;
            ///< @return combined armor rating of this actor

            virtual bool apply (const MWWorld::Ptr& ptr, const std::string& id,
                const MWWorld::Ptr& actor) const;
            ///< Apply \a id on \a ptr.
            /// \param actor Actor that is resposible for the ID being applied to \a ptr.
            /// \return Any effect?

            virtual void adjustScale (const MWWorld::Ptr &ptr, float &scale) const;

            virtual void skillUsageSucceeded (const MWWorld::Ptr& ptr, int skill, int usageType, float extraFactor=1.f) const;
            ///< Inform actor \a ptr that a skill use has succeeded.

            virtual bool isEssential (const MWWorld::Ptr& ptr) const;
            ///< Is \a ptr essential? (i.e. may losing \a ptr make the game unwinnable)

            virtual int getServices (const MWWorld::Ptr& actor) const;

            virtual bool isPersistent (const MWWorld::Ptr& ptr) const;

            virtual std::string getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const;

            static void registerSelf();

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;

            virtual int getSkill(const MWWorld::Ptr& ptr, int skill) const;

            /// Get a blood texture suitable for \a ptr (see Blood Texture 0-2 in Morrowind.ini)
            virtual int getBloodTexture (const MWWorld::Ptr& ptr) const;

            virtual bool isActor() const {
                return true;
            }

            virtual bool isNpc() const {
                return true;
            }

            virtual void readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
                const;
            ///< Read additional state from \a state into \a ptr.

            virtual void writeAdditionalState (const MWWorld::Ptr& ptr, ESM::ObjectState& state)
                const;
            ///< Write additional state from \a ptr into \a state.

            virtual int getBaseGold(const MWWorld::Ptr& ptr) const;

            virtual bool isClass(const MWWorld::Ptr& ptr, const std::string &className) const;

            virtual bool canSwim (const MWWorld::Ptr &ptr) const {
                return true;
            }

            virtual bool canWalk (const MWWorld::Ptr &ptr) const {
                return true;
            }

            virtual bool isBipedal (const MWWorld::Ptr &ptr) const;

            virtual void respawn (const MWWorld::Ptr& ptr) const;

            virtual void restock (const MWWorld::Ptr& ptr) const;

            virtual int getBaseFightRating (const MWWorld::Ptr& ptr) const;

            virtual std::string getPrimaryFaction(const MWWorld::Ptr &ptr) const;
            virtual int getPrimaryFactionRank(const MWWorld::Ptr &ptr) const;
    };
}

#endif
