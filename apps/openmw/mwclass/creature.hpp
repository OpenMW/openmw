#ifndef GAME_MWCLASS_CREATURE_H
#define GAME_MWCLASS_CREATURE_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Creature : public MWWorld::Class
    {
            void ensureCustomData (const MWWorld::Ptr& ptr) const;

            virtual MWWorld::Ptr
            copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const;

            static int getSndGenTypeFromName(const MWWorld::Ptr &ptr, const std::string &name);

            static const ESM::GameSetting *fMinWalkSpeedCreature;
            static const ESM::GameSetting *fMaxWalkSpeedCreature;
            static const ESM::GameSetting *fEncumberedMoveEffect;
            static const ESM::GameSetting *fSneakSpeedMultiplier;
            static const ESM::GameSetting *fAthleticsRunBonus;
            static const ESM::GameSetting *fBaseRunMultiplier;
            static const ESM::GameSetting *fMinFlySpeed;
            static const ESM::GameSetting *fMaxFlySpeed;
            static const ESM::GameSetting *fSwimRunBase;
            static const ESM::GameSetting *fSwimRunAthleticsMult;
            static const ESM::GameSetting *fKnockDownMult;
            static const ESM::GameSetting *iKnockDownOddsMult;
            static const ESM::GameSetting *iKnockDownOddsBase;


        public:

            virtual std::string getId (const MWWorld::Ptr& ptr) const;
            ///< Return ID of \a ptr

             virtual void insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const;

            virtual void adjustPosition(const MWWorld::Ptr& ptr) const;

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual bool hasToolTip (const MWWorld::Ptr& ptr) const;
            ///< @return true if this object has a tooltip when focused (default implementation: false)

            virtual MWGui::ToolTipInfo getToolTipInfo (const MWWorld::Ptr& ptr) const;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            virtual MWMechanics::CreatureStats& getCreatureStats (const MWWorld::Ptr& ptr) const;
            ///< Return creature stats

            virtual void hit(const MWWorld::Ptr& ptr, int type) const;

            virtual void block(const MWWorld::Ptr &ptr) const;

            virtual void onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, bool successful) const;

            virtual void setActorHealth(const MWWorld::Ptr& ptr, float health, const MWWorld::Ptr& attacker) const;

            virtual boost::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const;
            ///< Generate action for activation

            virtual MWWorld::ContainerStore& getContainerStore (
                const MWWorld::Ptr& ptr) const;
            ///< Return container store

            virtual MWWorld::InventoryStore& getInventoryStore (const MWWorld::Ptr& ptr) const;
            ///< Return inventory store

            virtual bool hasInventoryStore (const MWWorld::Ptr &ptr) const;

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;
            ///< Return name of the script attached to ptr

            virtual float getCapacity (const MWWorld::Ptr& ptr) const;
            ///< Return total weight that fits into the object. Throws an exception, if the object can't
            /// hold other objects.

            virtual float getEncumbrance (const MWWorld::Ptr& ptr) const;
            ///< Returns total weight of objects inside this object (including modifications from magic
            /// effects). Throws an exception, if the object can't hold other objects.

            virtual float getArmorRating (const MWWorld::Ptr& ptr) const;
            ///< @return combined armor rating of this actor

            virtual bool isEssential (const MWWorld::Ptr& ptr) const;
            ///< Is \a ptr essential? (i.e. may losing \a ptr make the game unwinnable)
            
            virtual int getServices (const MWWorld::Ptr& actor) const;

            virtual bool isPersistent (const MWWorld::Ptr& ptr) const;

            virtual std::string getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const;

            virtual MWMechanics::Movement& getMovementSettings (const MWWorld::Ptr& ptr) const;
            ///< Return desired movement.

            virtual Ogre::Vector3 getMovementVector (const MWWorld::Ptr& ptr) const;
            ///< Return desired movement vector (determined based on movement settings,
            /// stance and stats).

            virtual Ogre::Vector3 getRotationVector (const MWWorld::Ptr& ptr) const;
            ///< Return desired rotations, as euler angles.

            float getSpeed (const MWWorld::Ptr& ptr) const;

            static void registerSelf();

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;

            virtual bool
            isActor() const {
                return true;
            }

            virtual bool isFlying (const MWWorld::Ptr &ptr) const;

            virtual int getSkill(const MWWorld::Ptr &ptr, int skill) const;

            /// Get a blood texture suitable for \a ptr (see Blood Texture 0-2 in Morrowind.ini)
            virtual int getBloodTexture (const MWWorld::Ptr& ptr) const;
    };
}

#endif
