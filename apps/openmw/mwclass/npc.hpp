#ifndef GAME_MWCLASS_NPC_H
#define GAME_MWCLASS_NPC_H

#include "actor.hpp"

namespace ESM
{
    struct GameSetting;
}

namespace MWClass
{
    class Npc : public Actor
    {
            void ensureCustomData (const MWWorld::Ptr& ptr) const;

            MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const override;

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

            void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const override;
            ///< Add reference into a cell for rendering

            std::string getName (const MWWorld::ConstPtr& ptr) const override;
            ///< \return name or ID; can return an empty string.

            MWMechanics::CreatureStats& getCreatureStats (const MWWorld::Ptr& ptr) const override;
            ///< Return creature stats

            MWMechanics::NpcStats& getNpcStats (const MWWorld::Ptr& ptr) const override;
            ///< Return NPC stats

            MWWorld::ContainerStore& getContainerStore (const MWWorld::Ptr& ptr) const override;
            ///< Return container store

            bool hasToolTip(const MWWorld::ConstPtr& ptr) const override;
            ///< @return true if this object has a tooltip when focused (default implementation: true)

            MWGui::ToolTipInfo getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const override;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            MWWorld::InventoryStore& getInventoryStore (const MWWorld::Ptr& ptr) const override;
            ///< Return inventory store

            bool hasInventoryStore(const MWWorld::Ptr &ptr) const override { return true; }

            void hit(const MWWorld::Ptr& ptr, float attackStrength, int type) const override;

            void onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, const osg::Vec3f &hitPosition, bool successful) const override;

            void getModelsToPreload(const MWWorld::Ptr& ptr, std::vector<std::string>& models) const override;
            ///< Get a list of models to preload that this object may use (directly or indirectly). default implementation: list getModel().

            std::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const override;
            ///< Generate action for activation

            std::string getScript (const MWWorld::ConstPtr& ptr) const override;
            ///< Return name of the script attached to ptr

            float getMaxSpeed (const MWWorld::Ptr& ptr) const override;
            ///< Return maximal movement speed.

            float getJump(const MWWorld::Ptr &ptr) const override;
            ///< Return jump velocity (not accounting for movement)

            MWMechanics::Movement& getMovementSettings (const MWWorld::Ptr& ptr) const override;
            ///< Return desired movement.

            float getCapacity (const MWWorld::Ptr& ptr) const override;
            ///< Return total weight that fits into the object. Throws an exception, if the object can't
            /// hold other objects.

            float getEncumbrance (const MWWorld::Ptr& ptr) const override;
            ///< Returns total weight of objects inside this object (including modifications from magic
            /// effects). Throws an exception, if the object can't hold other objects.

            float getArmorRating (const MWWorld::Ptr& ptr) const override;
            ///< @return combined armor rating of this actor

            bool apply (const MWWorld::Ptr& ptr, const std::string& id,
                const MWWorld::Ptr& actor) const override;
            ///< Apply \a id on \a ptr.
            /// \param actor Actor that is resposible for the ID being applied to \a ptr.
            /// \return Any effect?

            void adjustScale (const MWWorld::ConstPtr &ptr, osg::Vec3f &scale, bool rendering) const override;
            /// @param rendering Indicates if the scale to adjust is for the rendering mesh, or for the collision mesh

            void skillUsageSucceeded (const MWWorld::Ptr& ptr, int skill, int usageType, float extraFactor=1.f) const override;
            ///< Inform actor \a ptr that a skill use has succeeded.

            bool isEssential (const MWWorld::ConstPtr& ptr) const override;
            ///< Is \a ptr essential? (i.e. may losing \a ptr make the game unwinnable)

            int getServices (const MWWorld::ConstPtr& actor) const override;

            bool isPersistent (const MWWorld::ConstPtr& ptr) const override;

            std::string getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const override;

            static void registerSelf();

            std::string getModel(const MWWorld::ConstPtr &ptr) const override;

            float getSkill(const MWWorld::Ptr& ptr, int skill) const override;

            /// Get a blood texture suitable for \a ptr (see Blood Texture 0-2 in Morrowind.ini)
            int getBloodTexture (const MWWorld::ConstPtr& ptr) const override;

            bool isNpc() const override
            {
                return true;
            }

            void readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const override;
            ///< Read additional state from \a state into \a ptr.

            void writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const override;
            ///< Write additional state from \a ptr into \a state.

            int getBaseGold(const MWWorld::ConstPtr& ptr) const override;

            bool isClass(const MWWorld::ConstPtr& ptr, const std::string &className) const override;

            bool canSwim (const MWWorld::ConstPtr &ptr) const override;

            bool canWalk (const MWWorld::ConstPtr &ptr) const override;

            bool isBipedal (const MWWorld::ConstPtr &ptr) const override;

            void respawn (const MWWorld::Ptr& ptr) const override;

            int getBaseFightRating (const MWWorld::ConstPtr& ptr) const override;

            std::string getPrimaryFaction(const MWWorld::ConstPtr &ptr) const override;
            int getPrimaryFactionRank(const MWWorld::ConstPtr &ptr) const override;

            void setBaseAISetting(const std::string& id, MWMechanics::CreatureStats::AiSetting setting, int value) const override;

            void modifyBaseInventory(const std::string& actorId, const std::string& itemId, int amount) const override;

            float getWalkSpeed(const MWWorld::Ptr& ptr) const override;

            float getRunSpeed(const MWWorld::Ptr& ptr) const override;

            float getSwimSpeed(const MWWorld::Ptr& ptr) const override;
    };
}

#endif
