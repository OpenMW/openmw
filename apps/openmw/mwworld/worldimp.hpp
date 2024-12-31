#ifndef GAME_MWWORLD_WORLDIMP_H
#define GAME_MWWORLD_WORLDIMP_H

#include <osg/Timer>
#include <osg/ref_ptr>

#include <components/esm3/readerscache.hpp>
#include <components/misc/rng.hpp>
#include <components/settings/settings.hpp>
#include <components/vfs/pathutil.hpp>

#include "../mwbase/world.hpp"

#include "contentloader.hpp"
#include "esmstore.hpp"
#include "globals.hpp"
#include "groundcoverstore.hpp"
#include "localscripts.hpp"
#include "ptr.hpp"
#include "scene.hpp"
#include "timestamp.hpp"
#include "worldmodel.hpp"

namespace osg
{
    class Group;
    class Stats;
}

namespace osgViewer
{
    class Viewer;
}

namespace Resource
{
    class ResourceSystem;
}

namespace SceneUtil
{
    class WorkQueue;
    class UnrefQueue;
}

namespace ESM
{
    struct Position;
    class RefId;
}

namespace Files
{
    class Collections;
}

namespace MWRender
{
    class SkyManager;
    class Animation;
    class Camera;
    class PostProcessor;
}

namespace ToUTF8
{
    class Utf8Encoder;
}

namespace MWPhysics
{
    class Object;
}

namespace MWWorld
{
    class DateTimeManager;
    class WeatherManager;
    class Player;
    class ProjectileManager;

    /// \brief The game world and its visual representation

    class World final : public MWBase::World
    {
    private:
        Resource::ResourceSystem* mResourceSystem;

        ESM::ReadersCache mReaders;
        MWWorld::ESMStore mStore;
        GroundcoverStore mGroundcoverStore;
        LocalScripts mLocalScripts;
        MWWorld::Globals mGlobalVariables;
        Misc::Rng::Generator mPrng;
        WorldModel mWorldModel;
        std::vector<int> mESMVersions; // the versions of esm files

        std::string mCurrentWorldSpace;

        std::unique_ptr<MWWorld::Player> mPlayer;
        std::unique_ptr<MWPhysics::PhysicsSystem> mPhysics;
        std::unique_ptr<DetourNavigator::Navigator> mNavigator;
        std::unique_ptr<MWRender::RenderingManager> mRendering;
        std::unique_ptr<MWWorld::Scene> mWorldScene;
        std::unique_ptr<MWWorld::WeatherManager> mWeatherManager;
        std::unique_ptr<MWWorld::DateTimeManager> mTimeManager;
        std::unique_ptr<ProjectileManager> mProjectileManager;

        bool mSky;
        bool mGodMode;
        bool mScriptsEnabled;
        bool mDiscardMovements;
        std::vector<std::string> mContentFiles;

        std::filesystem::path mUserDataPath;

        int mActivationDistanceOverride;

        std::string mStartCell;

        float mSwimHeightScale;

        float mDistanceToFacedObject;

        bool mTeleportEnabled;
        bool mLevitationEnabled;
        bool mGoToJail;
        int mDaysInPrison;
        bool mPlayerTraveling;
        bool mPlayerInJail;

        float mSpellPreloadTimer;

        std::map<MWWorld::Ptr, MWWorld::DoorState> mDoorStates;
        ///< only holds doors that are currently moving. 1 = opening, 2 = closing

        uint32_t mRandomSeed{};
        bool mIdsRebuilt{};

        // not implemented
        World(const World&) = delete;
        World& operator=(const World&) = delete;

        void updateWeather(float duration, bool paused = false);

        void initObjectInCell(const Ptr& ptr, CellStore& cell, bool adjustPos);
        Ptr moveObjectToCell(const Ptr& ptr, CellStore* cell, ESM::Position pos, bool adjustPos);
        Ptr copyObjectToCell(const ConstPtr& ptr, CellStore* cell, ESM::Position pos, int count, bool adjustPos);

        void updateSoundListener();

        void preloadSpells();

        MWWorld::Ptr getFacedObject(float maxDistance, bool ignorePlayer = true);

        void PCDropped(const Ptr& item);

        bool rotateDoor(const Ptr door, DoorState state, float duration);

        void processDoors(float duration);
        ///< Run physics simulation and modify \a world accordingly.

        void doPhysics(float duration, osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats);
        ///< Run physics simulation and modify \a world accordingly.

        void updateNavigator();

        void updateNavigatorObject(
            const MWPhysics::Object& object, const DetourNavigator::UpdateGuard* navigatorUpdateGuard = nullptr);

        void ensureNeededRecords();

        void fillGlobalVariables();

        void loadContentFiles(const Files::Collections& fileCollections, const std::vector<std::string>& content,
            ToUTF8::Utf8Encoder* encoder, Loading::Listener* listener);

        void loadGroundcoverFiles(const Files::Collections& fileCollections,
            const std::vector<std::string>& groundcoverFiles, ToUTF8::Utf8Encoder* encoder,
            Loading::Listener* listener);

        float feetToGameUnits(float feet);
        float getActivationDistancePlusTelekinesis();

        MWWorld::ConstPtr getClosestMarker(const MWWorld::ConstPtr& ptr, const ESM::RefId& id);
        MWWorld::ConstPtr getClosestMarkerFromExteriorPosition(const osg::Vec3f& worldPos, const ESM::RefId& id);

    public:
        WorldModel& getWorldModel() { return mWorldModel; }
        Scene& getWorldScene() { return *mWorldScene; }

        // FIXME
        void addContainerScripts(const Ptr& reference, CellStore* cell) override;
        void removeContainerScripts(const Ptr& reference) override;

        World(Resource::ResourceSystem* resourceSystem, int activationDistanceOverride, const std::string& startCell,
            const std::filesystem::path& userDataPath);

        void loadData(const Files::Collections& fileCollections, const std::vector<std::string>& contentFiles,
            const std::vector<std::string>& groundcoverFiles, ToUTF8::Utf8Encoder* encoder,
            Loading::Listener* listener);

        // Must be called after `loadData`.
        void init(osgViewer::Viewer* viewer, osg::ref_ptr<osg::Group> rootNode, SceneUtil::WorkQueue* workQueue,
            SceneUtil::UnrefQueue& unrefQueue);

        virtual ~World();

        void setRandomSeed(uint32_t seed) override;

        void startNewGame(bool bypass) override;
        ///< \param bypass Bypass regular game start.

        void clear() override;

        int countSavedGameRecords() const override;
        int countSavedGameCells() const override;

        void write(ESM::ESMWriter& writer, Loading::Listener& progress) const override;

        void readRecord(ESM::ESMReader& reader, uint32_t type) override;

        // switch to POV before showing player's death animation
        void useDeathCamera() override;

        void setWaterHeight(const float height) override;

        void rotateWorldObject(const MWWorld::Ptr& ptr, const osg::Quat& rotate) override;

        bool toggleWater() override;
        bool toggleWorld() override;
        bool toggleBorders() override;

        void adjustSky();

        Player& getPlayer() override;
        MWWorld::Ptr getPlayerPtr() override;
        MWWorld::ConstPtr getPlayerConstPtr() const override;

        MWWorld::ESMStore& getStore() override { return mStore; }

        const MWWorld::ESMStore& getStore() const override { return mStore; }

        const std::vector<int>& getESMVersions() const override;

        LocalScripts& getLocalScripts() override;

        bool isCellExterior() const override;

        bool isCellQuasiExterior() const override;

        ESM::RefId getCurrentWorldspace() const;

        void getDoorMarkers(MWWorld::CellStore& cell, std::vector<DoorMarker>& out) override;
        ///< get a list of teleport door markers for a given cell, to be displayed on the local map

        void setGlobalInt(GlobalVariableName name, int value) override;
        ///< Set value independently from real type.

        void setGlobalFloat(GlobalVariableName name, float value) override;
        ///< Set value independently from real type.

        int getGlobalInt(GlobalVariableName name) const override;
        ///< Get value independently from real type.

        float getGlobalFloat(GlobalVariableName name) const override;
        ///< Get value independently from real type.

        char getGlobalVariableType(GlobalVariableName name) const override;
        ///< Return ' ', if there is no global variable with this name.

        std::string_view getCellName(const MWWorld::CellStore* cell = nullptr) const override;
        ///< Return name of the cell.
        ///
        /// \note If cell==0, the cell the player is currently in will be used instead to
        /// generate a name.
        std::string_view getCellName(const MWWorld::Cell& cell) const override;

        void removeRefScript(const MWWorld::CellRef* ref) override;
        //< Remove the script attached to ref from mLocalScripts

        Ptr getPtr(const ESM::RefId& name, bool activeOnly) override;
        ///< Return a pointer to a liveCellRef with the given name.
        /// \param activeOnly do non search inactive cells.

        Ptr searchPtr(const ESM::RefId& name, bool activeOnly, bool searchInContainers = false) override;
        ///< Return a pointer to a liveCellRef with the given name.
        /// \param activeOnly do not search inactive cells.

        Ptr searchPtrViaActorId(int actorId) override;
        ///< Search is limited to the active cells.

        MWWorld::Ptr findContainer(const MWWorld::ConstPtr& ptr) override;
        ///< Return a pointer to a liveCellRef which contains \a ptr.
        /// \note Search is limited to the active cells.

        void adjustPosition(const Ptr& ptr, bool force) override;
        ///< Adjust position after load to be on ground. Must be called after model load.
        /// @param force do this even if the ptr is flying

        void fixPosition() override;
        ///< Attempt to fix position so that the player is not stuck inside the geometry.

        void enable(const Ptr& ptr) override;

        void disable(const Ptr& ptr) override;

        void advanceTime(double hours, bool incremental = false) override;
        ///< Advance in-game time.

        TimeStamp getTimeStamp() const override;
        ///< Return current in-game time and number of day since new game start.

        bool toggleSky() override;
        ///< \return Resulting mode

        void changeWeather(const ESM::RefId& region, const unsigned int id) override;

        int getCurrentWeather() const override;

        int getNextWeather() const override;

        float getWeatherTransition() const override;

        unsigned int getNightDayMode() const override;

        int getMasserPhase() const override;

        int getSecundaPhase() const override;

        void setMoonColour(bool red) override;

        void modRegion(const ESM::RefId& regionid, const std::vector<uint8_t>& chances) override;

        void changeToInteriorCell(const std::string_view cellName, const ESM::Position& position, bool adjustPlayerPos,
            bool changeEvent = true) override;
        ///< Move to interior cell.
        ///< @param changeEvent If false, do not trigger cell change flag or detect worldspace changes

        void changeToCell(const ESM::RefId& cellId, const ESM::Position& position, bool adjustPlayerPos,
            bool changeEvent = true) override;
        ///< @param changeEvent If false, do not trigger cell change flag or detect worldspace changes

        MWWorld::Ptr getFacedObject() override;
        ///< Return pointer to the object the player is looking at, if it is within activation range

        float getDistanceToFacedObject() override;

        /// @note No-op for items in containers. Use ContainerStore::removeItem instead.
        void deleteObject(const Ptr& ptr) override;

        void undeleteObject(const Ptr& ptr) override;

        MWWorld::Ptr moveObject(
            const Ptr& ptr, const osg::Vec3f& position, bool movePhysics = true, bool moveToActive = false) override;
        ///< @return an updated Ptr in case the Ptr's cell changes

        MWWorld::Ptr moveObject(const Ptr& ptr, CellStore* newCell, const osg::Vec3f& position, bool movePhysics = true,
            bool keepActive = false) override;
        ///< @return an updated Ptr

        MWWorld::Ptr moveObjectBy(const Ptr& ptr, const osg::Vec3f& vec, bool moveToActive) override;
        ///< @return an updated Ptr

        void scaleObject(const Ptr& ptr, float scale, bool force = false) override;

        /// World rotates object, uses radians
        /// @note Rotations via this method use a different rotation order than the initial rotations in the CS. This
        /// could be considered a bug, but is needed for MW compatibility.
        /// \param adjust indicates rotation should be set or adjusted
        void rotateObject(const Ptr& ptr, const osg::Vec3f& rot,
            MWBase::RotationFlags flags = MWBase::RotationFlag_inverseOrder) override;

        MWWorld::Ptr placeObject(
            const MWWorld::ConstPtr& ptr, MWWorld::CellStore* cell, const ESM::Position& pos) override;
        ///< Place an object. Makes a copy of the Ptr.

        MWWorld::Ptr safePlaceObject(const MWWorld::ConstPtr& ptr, const MWWorld::ConstPtr& referenceObject,
            MWWorld::CellStore* referenceCell, int direction, float distance) override;
        ///< Place an object in a safe place next to \a referenceObject. \a direction and \a distance specify the wanted
        ///< placement
        /// relative to \a referenceObject (but the object may be placed somewhere else if the wanted location is
        /// obstructed).

        float getMaxActivationDistance() const override;

        void queueMovement(const Ptr& ptr, const osg::Vec3f& velocity) override;
        ///< Queues movement for \a ptr (in local space), to be applied in the next call to
        /// doPhysics.

        void updateAnimatedCollisionShape(const Ptr& ptr) override;

        const MWPhysics::RayCastingInterface* getRayCasting() const override;

        bool castRenderingRay(MWPhysics::RayCastingResult& res, const osg::Vec3f& from, const osg::Vec3f& to,
            bool ignorePlayer, bool ignoreActors, std::span<const MWWorld::Ptr> ignoreList) override;

        void setActorCollisionMode(const Ptr& ptr, bool internal, bool external) override;
        bool isActorCollisionEnabled(const Ptr& ptr) override;

        bool toggleCollisionMode() override;
        ///< Toggle collision mode for player. If disabled player object should ignore
        /// collisions and gravity.
        ///< \return Resulting mode

        bool toggleRenderMode(MWRender::RenderMode mode) override;
        ///< Toggle a render mode.
        ///< \return Resulting mode

        void update(float duration, bool paused);
        void updatePhysics(
            float duration, bool paused, osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats);

        void updateWindowManager();

        MWWorld::Ptr placeObject(
            const MWWorld::Ptr& object, float cursorX, float cursorY, int amount, bool copy = true) override;
        ///< copy and place an object into the gameworld at the specified cursor position
        /// @param object
        /// @param cursor X (relative 0-1)
        /// @param cursor Y (relative 0-1)
        /// @param number of objects to place

        MWWorld::Ptr dropObjectOnGround(
            const MWWorld::Ptr& actor, const MWWorld::Ptr& object, int amount, bool copy = true) override;
        ///< copy and place an object into the gameworld at the given actor's position
        /// @param actor giving the dropped object position
        /// @param object
        /// @param number of objects to place

        bool canPlaceObject(float cursorX, float cursorY) override;
        ///< @return true if it is possible to place on object at specified cursor location

        void processChangedSettings(const Settings::CategorySettingVector& settings) override;

        bool isFlying(const MWWorld::Ptr& ptr) const override;
        bool isSlowFalling(const MWWorld::Ptr& ptr) const override;
        /// Is the head of the creature underwater?
        bool isSubmerged(const MWWorld::ConstPtr& object) const override;
        bool isSwimming(const MWWorld::ConstPtr& object) const override;
        bool isUnderwater(const MWWorld::CellStore* cell, const osg::Vec3f& pos) const override;
        bool isUnderwater(const MWWorld::ConstPtr& object, const float heightRatio) const override;
        bool isWading(const MWWorld::ConstPtr& object) const override;
        bool isWaterWalkingCastableOnTarget(const MWWorld::ConstPtr& target) const override;
        bool isOnGround(const MWWorld::Ptr& ptr) const override;

        osg::Matrixf getActorHeadTransform(const MWWorld::ConstPtr& actor) const override;

        void togglePOV(bool force = false) override;

        bool isFirstPerson() const override;
        bool isPreviewModeEnabled() const override;

        bool toggleVanityMode(bool enable) override;

        MWRender::Camera* getCamera() override;
        bool vanityRotateCamera(const float* rot) override;

        void applyDeferredPreviewRotationToPlayer(float dt) override;
        void disableDeferredPreviewRotation() override;

        void saveLoaded() override;

        void setupPlayer() override;
        void renderPlayer() override;

        /// open or close a non-teleport door (depending on current state)
        void activateDoor(const MWWorld::Ptr& door) override;

        /// update movement state of a non-teleport door as specified
        /// @param state see MWClass::setDoorState
        /// @note throws an exception when invoked on a teleport door
        void activateDoor(const MWWorld::Ptr& door, MWWorld::DoorState state) override;

        void getActorsStandingOn(const MWWorld::ConstPtr& object,
            std::vector<MWWorld::Ptr>& actors) override; ///< get a list of actors standing on \a object
        bool getPlayerStandingOn(
            const MWWorld::ConstPtr& object) override; ///< @return true if the player is standing on \a object
        bool getActorStandingOn(
            const MWWorld::ConstPtr& object) override; ///< @return true if any actor is standing on \a object
        bool getPlayerCollidingWith(
            const MWWorld::ConstPtr& object) override; ///< @return true if the player is colliding with \a object
        bool getActorCollidingWith(
            const MWWorld::ConstPtr& object) override; ///< @return true if any actor is colliding with \a object
        void hurtStandingActors(const MWWorld::ConstPtr& object, float dmgPerSecond) override;
        ///< Apply a health difference to any actors standing on \a object.
        /// To hurt actors, healthPerSecond should be a positive value. For a negative value, actors will be healed.
        void hurtCollidingActors(const MWWorld::ConstPtr& object, float dmgPerSecond) override;
        ///< Apply a health difference to any actors colliding with \a object.
        /// To hurt actors, healthPerSecond should be a positive value. For a negative value, actors will be healed.

        float getWindSpeed() override;

        void getContainersOwnedBy(const MWWorld::ConstPtr& npc, std::vector<MWWorld::Ptr>& out) override;
        ///< get all containers in active cells owned by this Npc
        void getItemsOwnedBy(const MWWorld::ConstPtr& npc, std::vector<MWWorld::Ptr>& out) override;
        ///< get all items in active cells owned by this Npc

        bool getLOS(const MWWorld::ConstPtr& actor, const MWWorld::ConstPtr& targetActor) override;
        ///< get Line of Sight (morrowind stupid implementation)

        float getDistToNearestRayHit(
            const osg::Vec3f& from, const osg::Vec3f& dir, float maxDist, bool includeWater = false) override;

        void enableActorCollision(const MWWorld::Ptr& actor, bool enable) override;

        RestPermitted canRest() const override;
        ///< check if the player is allowed to rest

        void rest(double hours) override;
        void rechargeItems(double duration, bool activeOnly);

        /// \todo Probably shouldn't be here
        MWRender::Animation* getAnimation(const MWWorld::Ptr& ptr) override;
        const MWRender::Animation* getAnimation(const MWWorld::ConstPtr& ptr) const override;
        void reattachPlayerCamera() override;

        /// \todo this does not belong here
        void screenshot(osg::Image* image, int w, int h) override;

        /// Find center of exterior cell above land surface
        /// \return false if exterior with given name not exists, true otherwise
        ESM::RefId findExteriorPosition(std::string_view nameId, ESM::Position& pos) override;

        /// Find position in interior cell near door entrance
        /// \return false if interior with given name not exists, true otherwise
        ESM::RefId findInteriorPosition(std::string_view name, ESM::Position& pos) override;

        /// Enables or disables use of teleport spell effects (recall, intervention, etc).
        void enableTeleporting(bool enable) override;

        /// Returns true if teleport spell effects are allowed.
        bool isTeleportingEnabled() const override;

        /// Enables or disables use of levitation spell effect.
        void enableLevitation(bool enable) override;

        /// Returns true if levitation spell effect is allowed.
        bool isLevitationEnabled() const override;

        bool getGodModeState() const override;

        bool toggleGodMode() override;

        bool toggleScripts() override;
        bool getScriptsEnabled() const override;

        /**
         * @brief startSpellCast attempt to start casting a spell. Might fail immediately if conditions are not met.
         * @param actor
         * @return Success or the failure condition.
         */
        MWWorld::SpellCastState startSpellCast(const MWWorld::Ptr& actor) override;

        /**
         * @brief Cast the actual spell, should be called mid-animation
         * @param actor
         */
        void castSpell(const MWWorld::Ptr& actor, bool manualSpell = false) override;

        void launchMagicBolt(const ESM::RefId& spellId, const MWWorld::Ptr& caster, const osg::Vec3f& fallbackDirection,
            ESM::RefNum item) override;
        void launchProjectile(MWWorld::Ptr& actor, MWWorld::Ptr& projectile, const osg::Vec3f& worldPos,
            const osg::Quat& orient, MWWorld::Ptr& bow, float speed, float attackStrength) override;
        void updateProjectilesCasters() override;

        void applyLoopingParticles(const MWWorld::Ptr& ptr) const override;

        const std::vector<std::string>& getContentFiles() const override;
        void breakInvisibility(const MWWorld::Ptr& actor) override;

        // Allow NPCs to use torches?
        bool useTorches() const override;

        float getSunVisibility() const override;
        float getSunPercentage() const override;

        bool findInteriorPositionInWorldSpace(const MWWorld::CellStore* cell, osg::Vec3f& result) override;

        /// Teleports \a ptr to the closest reference of \a id (e.g. DivineMarker, PrisonMarker, TempleMarker)
        /// @note id must be lower case
        void teleportToClosestMarker(const MWWorld::Ptr& ptr, const ESM::RefId& id) override;

        /// List all references (filtered by \a type) detected by \a ptr. The range
        /// is determined by the current magnitude of the "Detect X" magic effect belonging to \a type.
        /// @note This also works for references in containers.
        void listDetectedReferences(
            const MWWorld::Ptr& ptr, std::vector<MWWorld::Ptr>& out, DetectionType type) override;

        /// Update the value of some globals according to the world state, which may be used by dialogue entries.
        /// This should be called when initiating a dialogue.
        void updateDialogueGlobals() override;

        /// Moves all stolen items from \a ptr to the closest evidence chest.
        void confiscateStolenItems(const MWWorld::Ptr& ptr) override;

        void goToJail() override;

        /// Spawn a random creature from a levelled list next to the player
        void spawnRandomCreature(const ESM::RefId& creatureList) override;

        /// Spawn a blood effect for \a ptr at \a worldPosition
        void spawnBloodEffect(const MWWorld::Ptr& ptr, const osg::Vec3f& worldPosition) override;

        void spawnEffect(VFS::Path::NormalizedView model, const std::string& textureOverride,
            const osg::Vec3f& worldPos, float scale = 1.f, bool isMagicVFX = true,
            bool useAmbientLight = true) override;

        /// @see MWWorld::WeatherManager::isInStorm
        bool isInStorm() const override;

        /// @see MWWorld::WeatherManager::getStormDirection
        osg::Vec3f getStormDirection() const override;

        /// Resets all actors in the current active cells to their original location within that cell.
        void resetActors() override;

        bool isWalkingOnWater(const MWWorld::ConstPtr& actor) const override;

        /// Return a vector aiming the actor's weapon towards a target.
        /// @note The length of the vector is the distance between actor and target.
        osg::Vec3f aimToTarget(
            const MWWorld::ConstPtr& actor, const MWWorld::ConstPtr& target, bool isRangedCombat) override;

        bool isPlayerInJail() const override;

        void setPlayerTraveling(bool traveling) override;
        bool isPlayerTraveling() const override;

        /// Return terrain height at \a worldPos position.
        float getTerrainHeightAt(const osg::Vec3f& worldPos, ESM::RefId worldspace) const override;

        /// Return physical or rendering half extents of the given actor.
        osg::Vec3f getHalfExtents(const MWWorld::ConstPtr& actor, bool rendering = false) const override;

        /// Export scene graph to a file and return the filename.
        /// \param ptr object to export scene graph for (if empty, export entire scene graph)
        std::filesystem::path exportSceneGraph(const MWWorld::Ptr& ptr) override;

        /// Preload VFX associated with this effect list
        void preloadEffects(const ESM::EffectList* effectList) override;

        DetourNavigator::Navigator* getNavigator() const override;

        void updateActorPath(const MWWorld::ConstPtr& actor, const std::deque<osg::Vec3f>& path,
            const DetourNavigator::AgentBounds& agentBounds, const osg::Vec3f& start,
            const osg::Vec3f& end) const override;

        void removeActorPath(const MWWorld::ConstPtr& actor) const override;

        void setNavMeshNumberToRender(const std::size_t value) override;

        DetourNavigator::AgentBounds getPathfindingAgentBounds(const MWWorld::ConstPtr& actor) const override;

        bool hasCollisionWithDoor(
            const MWWorld::ConstPtr& door, const osg::Vec3f& position, const osg::Vec3f& destination) const override;

        bool isAreaOccupiedByOtherActor(const osg::Vec3f& position, const float radius,
            std::span<const MWWorld::ConstPtr> ignore, std::vector<MWWorld::Ptr>* occupyingActors) const override;

        void reportStats(unsigned int frameNumber, osg::Stats& stats) const override;

        std::vector<MWWorld::Ptr> getAll(const ESM::RefId& id) override;

        Misc::Rng::Generator& getPrng() override;

        MWRender::RenderingManager* getRenderingManager() override { return mRendering.get(); }

        MWRender::PostProcessor* getPostProcessor() override;

        DateTimeManager* getTimeManager() override { return mTimeManager.get(); }

        void setActorActive(const MWWorld::Ptr& ptr, bool value) override;
    };
}

#endif
