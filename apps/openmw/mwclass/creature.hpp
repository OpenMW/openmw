#ifndef GAME_MWCLASS_CREATURE_H
#define GAME_MWCLASS_CREATURE_H

#include "actor.hpp"

namespace ESM
{
    struct GameSetting;
}

namespace MWClass
{
    class Creature : public Actor
    {
            void ensureCustomData (const MWWorld::Ptr& ptr) const;

            MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const override;

            static int getSndGenTypeFromName(const MWWorld::Ptr &ptr, const std::string &name);

            // cached GMSTs
            struct GMST
            {
                const ESM::GameSetting *fMinWalkSpeedCreature;
                const ESM::GameSetting *fMaxWalkSpeedCreature;
                const ESM::GameSetting *fEncumberedMoveEffect;
                const ESM::GameSetting *fSneakSpeedMultiplier;
                const ESM::GameSetting *fAthleticsRunBonus;
                const ESM::GameSetting *fBaseRunMultiplier;
                const ESM::GameSetting *fMinFlySpeed;
                const ESM::GameSetting *fMaxFlySpeed;
                const ESM::GameSetting *fSwimRunBase;
                const ESM::GameSetting *fSwimRunAthleticsMult;
                const ESM::GameSetting *fKnockDownMult;
                const ESM::GameSetting *iKnockDownOddsMult;
                const ESM::GameSetting *iKnockDownOddsBase;
            };

            static const GMST& getGmst();

        public:

             void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const override;
            ///< Add reference into a cell for rendering

            std::string getName (const MWWorld::ConstPtr& ptr) const override;
            ///< \return name or ID; can return an empty string.

            bool hasToolTip(const MWWorld::ConstPtr& ptr) const override;
            ///< @return true if this object has a tooltip when focused (default implementation: true)

            MWGui::ToolTipInfo getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const override;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            MWMechanics::CreatureStats& getCreatureStats (const MWWorld::Ptr& ptr) const override;
            ///< Return creature stats

            void hit(const MWWorld::Ptr& ptr, float attackStrength, int type) const override;

            void onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, const osg::Vec3f &hitPosition, bool successful) const override;

            std::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const override;
            ///< Generate action for activation

            MWWorld::ContainerStore& getContainerStore (
                const MWWorld::Ptr& ptr) const override;
            ///< Return container store

            MWWorld::InventoryStore& getInventoryStore (const MWWorld::Ptr& ptr) const override;
            ///< Return inventory store

            bool hasInventoryStore (const MWWorld::Ptr &ptr) const override;

            std::string getScript (const MWWorld::ConstPtr& ptr) const override;
            ///< Return name of the script attached to ptr

            float getCapacity (const MWWorld::Ptr& ptr) const override;
            ///< Return total weight that fits into the object. Throws an exception, if the object can't
            /// hold other objects.

            float getArmorRating (const MWWorld::Ptr& ptr) const override;
            ///< @return combined armor rating of this actor

            bool isEssential (const MWWorld::ConstPtr& ptr) const override;
            ///< Is \a ptr essential? (i.e. may losing \a ptr make the game unwinnable)

            int getServices (const MWWorld::ConstPtr& actor) const override;

            bool isPersistent (const MWWorld::ConstPtr& ptr) const override;

            std::string getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const override;

            MWMechanics::Movement& getMovementSettings (const MWWorld::Ptr& ptr) const override;
            ///< Return desired movement.

            float getMaxSpeed (const MWWorld::Ptr& ptr) const override;

            static void registerSelf();

            std::string getModel(const MWWorld::ConstPtr &ptr) const override;

            void getModelsToPreload(const MWWorld::Ptr& ptr, std::vector<std::string>& models) const override;
            ///< Get a list of models to preload that this object may use (directly or indirectly). default implementation: list getModel().

            bool isBipedal (const MWWorld::ConstPtr &ptr) const override;
            bool canFly (const MWWorld::ConstPtr &ptr) const override;
            bool canSwim (const MWWorld::ConstPtr &ptr) const override;
            bool canWalk (const MWWorld::ConstPtr &ptr) const override;

            float getSkill(const MWWorld::Ptr &ptr, int skill) const override;

            /// Get a blood texture suitable for \a ptr (see Blood Texture 0-2 in Morrowind.ini)
            int getBloodTexture (const MWWorld::ConstPtr& ptr) const override;

            void readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const override;
            ///< Read additional state from \a state into \a ptr.

            void writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const override;
            ///< Write additional state from \a ptr into \a state.

            int getBaseGold(const MWWorld::ConstPtr& ptr) const override;

            void respawn (const MWWorld::Ptr& ptr) const override;

            int getBaseFightRating(const MWWorld::ConstPtr &ptr) const override;

            void adjustScale(const MWWorld::ConstPtr& ptr, osg::Vec3f& scale, bool rendering) const override;
            /// @param rendering Indicates if the scale to adjust is for the rendering mesh, or for the collision mesh

            void setBaseAISetting(const std::string& id, MWMechanics::CreatureStats::AiSetting setting, int value) const override;

            void modifyBaseInventory(const std::string& actorId, const std::string& itemId, int amount) const override;

            float getWalkSpeed(const MWWorld::Ptr& ptr) const override;

            float getRunSpeed(const MWWorld::Ptr& ptr) const override;

            float getSwimSpeed(const MWWorld::Ptr& ptr) const override;
    };
}

#endif
