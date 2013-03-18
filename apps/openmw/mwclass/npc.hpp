#ifndef GAME_MWCLASS_NPC_H
#define GAME_MWCLASS_NPC_H

#include "../mwworld/class.hpp"

namespace ESM
{
    class GameSetting;
}

namespace MWClass
{
    class Npc : public MWWorld::Class
    {
            void ensureCustomData (const MWWorld::Ptr& ptr) const;

            virtual MWWorld::Ptr
            copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const;

            static const ESM::GameSetting *fMinWalkSpeed;
            static const ESM::GameSetting *fMaxWalkSpeed;
            static const ESM::GameSetting *fEncumberedMoveEffect;
            static const ESM::GameSetting *fSneakSpeedMultiplier;
            static const ESM::GameSetting *fAthleticsRunBonus;
            static const ESM::GameSetting *fBaseRunMultiplier;
            static const ESM::GameSetting *fMinFlySpeed;
            static const ESM::GameSetting *fMaxFlySpeed;
            static const ESM::GameSetting *fSwimRunBase;
            static const ESM::GameSetting *fSwimRunAthleticsMult;
            static const ESM::GameSetting *fJumpEncumbranceBase;
            static const ESM::GameSetting *fJumpEncumbranceMultiplier;
            static const ESM::GameSetting *fJumpAcrobaticsBase;
            static const ESM::GameSetting *fJumpAcroMultiplier;
            static const ESM::GameSetting *fJumpRunMultiplier;
            static const ESM::GameSetting *fWereWolfRunMult;

        public:

            virtual std::string getId (const MWWorld::Ptr& ptr) const;
            ///< Return ID of \a ptr

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const;

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

            virtual boost::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const;
            ///< Generate action for activation

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;
            ///< Return name of the script attached to ptr

            virtual void setForceStance (const MWWorld::Ptr& ptr, Stance stance, bool force) const;
            ///< Force or unforce a stance.

            virtual void setStance (const MWWorld::Ptr& ptr, Stance stance, bool set) const;
            ///< Set or unset a stance.

            virtual bool getStance (const MWWorld::Ptr& ptr, Stance stance, bool ignoreForce = false)
                const;
            ///< Check if a stance is active or not.

            virtual float getSpeed (const MWWorld::Ptr& ptr) const;
            ///< Return movement speed.

            virtual float getJump(const MWWorld::Ptr &ptr) const;
            ///< Return jump velocity (not accounting for movement)

            virtual MWMechanics::Movement& getMovementSettings (const MWWorld::Ptr& ptr) const;
            ///< Return desired movement.

            virtual Ogre::Vector3 getMovementVector (const MWWorld::Ptr& ptr) const;
            ///< Return desired movement vector (determined based on movement settings,
            /// stance and stats).

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

            virtual void skillUsageSucceeded (const MWWorld::Ptr& ptr, int skill, int usageType) const;
            ///< Inform actor \a ptr that a skill use has succeeded.

            virtual void adjustRotation(const MWWorld::Ptr& ptr,float& x,float& y,float& z) const;

            virtual bool isEssential (const MWWorld::Ptr& ptr) const;
            ///< Is \a ptr essential? (i.e. may losing \a ptr make the game unwinnable)
            
            static void registerSelf();

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;

            virtual bool
            isActor() const {
                return true;
            }
    };
}

#endif
