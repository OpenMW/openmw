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

            virtual MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const;

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

             virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual std::string getName (const MWWorld::ConstPtr& ptr) const;
            ///< \return name or ID; can return an empty string.

            virtual bool hasToolTip(const MWWorld::ConstPtr& ptr) const;
            ///< @return true if this object has a tooltip when focused (default implementation: true)

            virtual MWGui::ToolTipInfo getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            virtual MWMechanics::CreatureStats& getCreatureStats (const MWWorld::Ptr& ptr) const;
            ///< Return creature stats

            virtual void hit(const MWWorld::Ptr& ptr, float attackStrength, int type) const;

            virtual void onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, const osg::Vec3f &hitPosition, bool successful) const;

            virtual std::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const;
            ///< Generate action for activation

            virtual MWWorld::ContainerStore& getContainerStore (
                const MWWorld::Ptr& ptr) const;
            ///< Return container store

            virtual MWWorld::InventoryStore& getInventoryStore (const MWWorld::Ptr& ptr) const;
            ///< Return inventory store

            virtual bool hasInventoryStore (const MWWorld::Ptr &ptr) const;

            virtual std::string getScript (const MWWorld::ConstPtr& ptr) const;
            ///< Return name of the script attached to ptr

            virtual float getCapacity (const MWWorld::Ptr& ptr) const;
            ///< Return total weight that fits into the object. Throws an exception, if the object can't
            /// hold other objects.

            virtual float getArmorRating (const MWWorld::Ptr& ptr) const;
            ///< @return combined armor rating of this actor

            virtual bool isEssential (const MWWorld::ConstPtr& ptr) const;
            ///< Is \a ptr essential? (i.e. may losing \a ptr make the game unwinnable)

            virtual int getServices (const MWWorld::ConstPtr& actor) const;

            virtual bool isPersistent (const MWWorld::ConstPtr& ptr) const;

            virtual std::string getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const;

            virtual MWMechanics::Movement& getMovementSettings (const MWWorld::Ptr& ptr) const;
            ///< Return desired movement.

            float getSpeed (const MWWorld::Ptr& ptr) const;

            static void registerSelf();

            virtual std::string getModel(const MWWorld::ConstPtr &ptr) const;

            virtual void getModelsToPreload(const MWWorld::Ptr& ptr, std::vector<std::string>& models) const;
            ///< Get a list of models to preload that this object may use (directly or indirectly). default implementation: list getModel().

            virtual bool isBipedal (const MWWorld::ConstPtr &ptr) const;
            virtual bool canFly (const MWWorld::ConstPtr &ptr) const;
            virtual bool canSwim (const MWWorld::ConstPtr &ptr) const;
            virtual bool canWalk (const MWWorld::ConstPtr &ptr) const;

            virtual int getSkill(const MWWorld::Ptr &ptr, int skill) const;

            /// Get a blood texture suitable for \a ptr (see Blood Texture 0-2 in Morrowind.ini)
            virtual int getBloodTexture (const MWWorld::ConstPtr& ptr) const;

            virtual void readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const;
            ///< Read additional state from \a state into \a ptr.

            virtual void writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const;
            ///< Write additional state from \a ptr into \a state.

            virtual int getBaseGold(const MWWorld::ConstPtr& ptr) const;

            virtual void respawn (const MWWorld::Ptr& ptr) const;

            virtual void restock (const MWWorld::Ptr &ptr) const;

            virtual int getBaseFightRating(const MWWorld::ConstPtr &ptr) const;

            virtual void adjustScale(const MWWorld::ConstPtr& ptr, osg::Vec3f& scale, bool rendering) const;
            /// @param rendering Indicates if the scale to adjust is for the rendering mesh, or for the collision mesh
    };
}

#endif
