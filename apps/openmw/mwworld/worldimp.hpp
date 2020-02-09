#ifndef GAME_MWWORLD_WORLDIMP_H
#define GAME_MWWORLD_WORLDIMP_H

#include <osg/ref_ptr>

#include <components/settings/settings.hpp>
#include <components/fallback/fallback.hpp>

#include "../mwbase/world.hpp"

#include "ptr.hpp"
#include "scene.hpp"
#include "esmstore.hpp"
#include "cells.hpp"
#include "localscripts.hpp"
#include "timestamp.hpp"
#include "globals.hpp"
#include "contentloader.hpp"

namespace osg
{
    class Group;
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
}

namespace ESM
{
    struct Position;
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
}

namespace ToUTF8
{
    class Utf8Encoder;
}

struct ContentLoader;

namespace MWPhysics
{
    class Object;
}

namespace MWWorld
{
    class WeatherManager;
    class Player;
    class ProjectileManager;

    /// \brief The game world and its visual representation

    class World final: public MWBase::World
    {
            Resource::ResourceSystem* mResourceSystem;

            std::vector<ESM::ESMReader> mEsm;
            MWWorld::ESMStore mStore;
            LocalScripts mLocalScripts;
            MWWorld::Globals mGlobalVariables;
            bool mSky;

            ESM::Variant* mGameHour;
            ESM::Variant* mDaysPassed;
            ESM::Variant* mDay;
            ESM::Variant* mMonth;
            ESM::Variant* mYear;
            ESM::Variant* mTimeScale;

            Cells mCells;

            std::string mCurrentWorldSpace;

            std::unique_ptr<MWWorld::Player> mPlayer;
            std::unique_ptr<MWPhysics::PhysicsSystem> mPhysics;
            std::unique_ptr<DetourNavigator::Navigator> mNavigator;
            std::unique_ptr<MWRender::RenderingManager> mRendering;
            std::unique_ptr<MWWorld::Scene> mWorldScene;
            std::unique_ptr<MWWorld::WeatherManager> mWeatherManager;
            std::shared_ptr<ProjectileManager> mProjectileManager;

            bool mGodMode;
            bool mScriptsEnabled;
            std::vector<std::string> mContentFiles;

            std::string mUserDataPath;

            osg::Vec3f mDefaultHalfExtents;
            bool mShouldUpdateNavigator = false;

            // not implemented
            World (const World&);
            World& operator= (const World&);

            int mActivationDistanceOverride;

            std::map<MWWorld::Ptr, MWWorld::DoorState> mDoorStates;
            ///< only holds doors that are currently moving. 1 = opening, 2 = closing

            std::string mStartCell;

            void updateWeather(float duration, bool paused = false);
            int getDaysPerMonth (int month) const;

            void rotateObjectImp (const Ptr& ptr, const osg::Vec3f& rot, MWBase::RotationFlags flags);

            Ptr moveObjectImp (const Ptr& ptr, float x, float y, float z, bool movePhysics=true, bool moveToActive=false);
            ///< @return an updated Ptr in case the Ptr's cell changes

            Ptr copyObjectToCell(const ConstPtr &ptr, CellStore* cell, ESM::Position pos, int count, bool adjustPos);

            void updateSoundListener();
            void updatePlayer();

            void preloadSpells();

            MWWorld::Ptr getFacedObject(float maxDistance, bool ignorePlayer=true);

    public: // FIXME
            void addContainerScripts(const Ptr& reference, CellStore* cell) override;
            void removeContainerScripts(const Ptr& reference) override;
    private:
            void PCDropped (const Ptr& item);

            bool rotateDoor(const Ptr door, DoorState state, float duration);

            void processDoors(float duration);
            ///< Run physics simulation and modify \a world accordingly.

            void doPhysics(float duration);
            ///< Run physics simulation and modify \a world accordingly.

            void updateNavigator();

            bool updateNavigatorObject(const MWPhysics::Object* object);

            void ensureNeededRecords();

            void fillGlobalVariables();

            /**
             * @brief loadContentFiles - Loads content files (esm,esp,omwgame,omwaddon)
             * @param fileCollections- Container which holds content file names and their paths
             * @param content - Container which holds content file names
             * @param contentLoader -
             */
            void loadContentFiles(const Files::Collections& fileCollections,
                const std::vector<std::string>& content, ContentLoader& contentLoader);

            float mSwimHeightScale;

            float mDistanceToFacedObject;

            bool mTeleportEnabled;
            bool mLevitationEnabled;
            bool mGoToJail;
            int mDaysInPrison;
            bool mPlayerTraveling;
            bool mPlayerInJail;

            float mSpellPreloadTimer;

            float feetToGameUnits(float feet);
            float getActivationDistancePlusTelekinesis();

            MWWorld::ConstPtr getClosestMarker( const MWWorld::Ptr &ptr, const std::string &id );
            MWWorld::ConstPtr getClosestMarkerFromExteriorPosition( const osg::Vec3f& worldPos, const std::string &id );

        public:

            World (
                osgViewer::Viewer* viewer,
                osg::ref_ptr<osg::Group> rootNode,
                Resource::ResourceSystem* resourceSystem, SceneUtil::WorkQueue* workQueue,
                const Files::Collections& fileCollections,
                const std::vector<std::string>& contentFiles,
                ToUTF8::Utf8Encoder* encoder, int activationDistanceOverride,
                const std::string& startCell, const std::string& startupScript,
                const std::string& resourcePath, const std::string& userDataPath);

            virtual ~World();

            void startNewGame (bool bypass) override;
            ///< \param bypass Bypass regular game start.

            void clear() override;

            int countSavedGameRecords() const override;
            int countSavedGameCells() const override;

            void write (ESM::ESMWriter& writer, Loading::Listener& progress) const override;

            void readRecord (ESM::ESMReader& reader, uint32_t type,
                const std::map<int, int>& contentFileMap) override;

            CellStore *getExterior (int x, int y) override;

            CellStore *getInterior (const std::string& name) override;

            CellStore *getCell (const ESM::CellId& id) override;

            void testExteriorCells() override;
            void testInteriorCells() override;

            //switch to POV before showing player's death animation
            void useDeathCamera() override;

            void setWaterHeight(const float height) override;

            void rotateWorldObject (const MWWorld::Ptr& ptr, osg::Quat rotate) override;

            bool toggleWater() override;
            bool toggleWorld() override;
            bool toggleBorders() override;

            void adjustSky() override;

            Player& getPlayer() override;
            MWWorld::Ptr getPlayerPtr() override;
            MWWorld::ConstPtr getPlayerConstPtr() const override;

            const MWWorld::ESMStore& getStore() const override;

            std::vector<ESM::ESMReader>& getEsmReader() override;

            LocalScripts& getLocalScripts() override;

            bool hasCellChanged() const override;
            ///< Has the set of active cells changed, since the last frame?

            bool isCellExterior() const override;

            bool isCellQuasiExterior() const override;

            osg::Vec2f getNorthVector (const CellStore* cell) override;
            ///< get north vector for given interior cell

            void getDoorMarkers (MWWorld::CellStore* cell, std::vector<DoorMarker>& out) override;
            ///< get a list of teleport door markers for a given cell, to be displayed on the local map

            void setGlobalInt (const std::string& name, int value) override;
            ///< Set value independently from real type.

            void setGlobalFloat (const std::string& name, float value) override;
            ///< Set value independently from real type.

            int getGlobalInt (const std::string& name) const override;
            ///< Get value independently from real type.

            float getGlobalFloat (const std::string& name) const override;
            ///< Get value independently from real type.

            char getGlobalVariableType (const std::string& name) const override;
            ///< Return ' ', if there is no global variable with this name.

            std::string getCellName (const MWWorld::CellStore *cell = 0) const override;
            ///< Return name of the cell.
            ///
            /// \note If cell==0, the cell the player is currently in will be used instead to
            /// generate a name.

            void removeRefScript (MWWorld::RefData *ref) override;
            //< Remove the script attached to ref from mLocalScripts

            Ptr getPtr (const std::string& name, bool activeOnly) override;
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            Ptr searchPtr (const std::string& name, bool activeOnly, bool searchInContainers = true) override;
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            Ptr searchPtrViaActorId (int actorId) override;
            ///< Search is limited to the active cells.

            MWWorld::Ptr findContainer (const MWWorld::ConstPtr& ptr) override;
            ///< Return a pointer to a liveCellRef which contains \a ptr.
            /// \note Search is limited to the active cells.

            void adjustPosition (const Ptr& ptr, bool force) override;
            ///< Adjust position after load to be on ground. Must be called after model load.
            /// @param force do this even if the ptr is flying

            void fixPosition () override;
            ///< Attempt to fix position so that the player is not stuck inside the geometry.

            void enable (const Ptr& ptr) override;

            void disable (const Ptr& ptr) override;

            void advanceTime (double hours, bool incremental = false) override;
            ///< Advance in-game time.

            void setHour (double hour) override;
            ///< Set in-game time hour.

            void setMonth (int month) override;
            ///< Set in-game time month.

            void setDay (int day) override;
            ///< Set in-game time day.

            int getDay() const override;
            int getMonth() const override;
            int getYear() const override;

            std::string getMonthName (int month = -1) const override;
            ///< Return name of month (-1: current month)

            TimeStamp getTimeStamp() const override;
            ///< Return current in-game time stamp.

            bool toggleSky() override;
            ///< \return Resulting mode

            void changeWeather (const std::string& region, const unsigned int id) override;

            int getCurrentWeather() const override;

            unsigned int getNightDayMode() const override;

            int getMasserPhase() const override;

            int getSecundaPhase() const override;

            void setMoonColour (bool red) override;

            void modRegion(const std::string &regionid, const std::vector<char> &chances) override;

            float getTimeScaleFactor() const override;

            void changeToInteriorCell (const std::string& cellName, const ESM::Position& position, bool adjustPlayerPos, bool changeEvent = true) override;
            ///< Move to interior cell.
            ///< @param changeEvent If false, do not trigger cell change flag or detect worldspace changes

            void changeToExteriorCell (const ESM::Position& position, bool adjustPlayerPos, bool changeEvent = true) override;
            ///< Move to exterior cell.
            ///< @param changeEvent If false, do not trigger cell change flag or detect worldspace changes

            void changeToCell (const ESM::CellId& cellId, const ESM::Position& position, bool adjustPlayerPos, bool changeEvent=true) override;
            ///< @param changeEvent If false, do not trigger cell change flag or detect worldspace changes

            const ESM::Cell *getExterior (const std::string& cellName) const override;
            ///< Return a cell matching the given name or a 0-pointer, if there is no such cell.

            void markCellAsUnchanged() override;

            MWWorld::Ptr getFacedObject() override;
            ///< Return pointer to the object the player is looking at, if it is within activation range

            float getDistanceToFacedObject() override;

            /// Returns a pointer to the object the provided object would hit (if within the
            /// specified distance), and the point where the hit occurs. This will attempt to
            /// use the "Head" node as a basis.
            std::pair<MWWorld::Ptr,osg::Vec3f> getHitContact(const MWWorld::ConstPtr &ptr, float distance, std::vector<MWWorld::Ptr> &targets) override;

            /// @note No-op for items in containers. Use ContainerStore::removeItem instead.
            void deleteObject (const Ptr& ptr) override;

            void undeleteObject (const Ptr& ptr) override;

            MWWorld::Ptr moveObject (const Ptr& ptr, float x, float y, float z, bool moveToActive=false) override;
            ///< @return an updated Ptr in case the Ptr's cell changes

            MWWorld::Ptr moveObject (const Ptr& ptr, CellStore* newCell, float x, float y, float z, bool movePhysics=true) override;
            ///< @return an updated Ptr

            void scaleObject (const Ptr& ptr, float scale) override;

            /// World rotates object, uses radians
            /// @note Rotations via this method use a different rotation order than the initial rotations in the CS. This
            /// could be considered a bug, but is needed for MW compatibility.
            /// \param adjust indicates rotation should be set or adjusted
            void rotateObject (const Ptr& ptr, float x, float y, float z,
                MWBase::RotationFlags flags = MWBase::RotationFlag_inverseOrder) override;

            MWWorld::Ptr placeObject(const MWWorld::ConstPtr& ptr, MWWorld::CellStore* cell, ESM::Position pos) override;
            ///< Place an object. Makes a copy of the Ptr.

            MWWorld::Ptr safePlaceObject (const MWWorld::ConstPtr& ptr, const MWWorld::ConstPtr& referenceObject, MWWorld::CellStore* referenceCell, int direction, float distance) override;
            ///< Place an object in a safe place next to \a referenceObject. \a direction and \a distance specify the wanted placement
            /// relative to \a referenceObject (but the object may be placed somewhere else if the wanted location is obstructed).

            float getMaxActivationDistance() override;

            void indexToPosition (int cellX, int cellY, float &x, float &y, bool centre = false)
                const override;
            ///< Convert cell numbers to position.

            void positionToIndex (float x, float y, int &cellX, int &cellY) const override;
            ///< Convert position to cell numbers

            void queueMovement(const Ptr &ptr, const osg::Vec3f &velocity) override;
            ///< Queues movement for \a ptr (in local space), to be applied in the next call to
            /// doPhysics.

            void updateAnimatedCollisionShape(const Ptr &ptr) override;

            bool castRay (float x1, float y1, float z1, float x2, float y2, float z2, int mask) override;
            ///< cast a Ray and return true if there is an object in the ray path.

            bool castRay (float x1, float y1, float z1, float x2, float y2, float z2) override;

            bool castRay(const osg::Vec3f& from, const osg::Vec3f& to, int mask, const MWWorld::ConstPtr& ignore) override;

            void setActorCollisionMode(const Ptr& ptr, bool internal, bool external) override;
            bool isActorCollisionEnabled(const Ptr& ptr) override;

            bool toggleCollisionMode() override;
            ///< Toggle collision mode for player. If disabled player object should ignore
            /// collisions and gravity.
            ///< \return Resulting mode

            bool toggleRenderMode (MWRender::RenderMode mode) override;
            ///< Toggle a render mode.
            ///< \return Resulting mode

            const ESM::Potion *createRecord (const ESM::Potion& record) override;
            ///< Create a new record (of type potion) in the ESM store.
            /// \return pointer to created record

            const ESM::Spell *createRecord (const ESM::Spell& record) override;
            ///< Create a new record (of type spell) in the ESM store.
            /// \return pointer to created record

            const ESM::Class *createRecord (const ESM::Class& record) override;
            ///< Create a new record (of type class) in the ESM store.
            /// \return pointer to created record

            const ESM::Cell *createRecord (const ESM::Cell& record) override;
            ///< Create a new record (of type cell) in the ESM store.
            /// \return pointer to created record

            const ESM::NPC *createRecord(const ESM::NPC &record) override;
            ///< Create a new record (of type npc) in the ESM store.
            /// \return pointer to created record

            const ESM::Armor *createRecord (const ESM::Armor& record) override;
            ///< Create a new record (of type armor) in the ESM store.
            /// \return pointer to created record

            const ESM::Weapon *createRecord (const ESM::Weapon& record) override;
            ///< Create a new record (of type weapon) in the ESM store.
            /// \return pointer to created record

            const ESM::Clothing *createRecord (const ESM::Clothing& record) override;
            ///< Create a new record (of type clothing) in the ESM store.
            /// \return pointer to created record

            const ESM::Enchantment *createRecord (const ESM::Enchantment& record) override;
            ///< Create a new record (of type enchantment) in the ESM store.
            /// \return pointer to created record

            const ESM::Book *createRecord (const ESM::Book& record) override;
            ///< Create a new record (of type book) in the ESM store.
            /// \return pointer to created record

            const ESM::CreatureLevList *createOverrideRecord (const ESM::CreatureLevList& record) override;
            ///< Write this record to the ESM store, allowing it to override a pre-existing record with the same ID.
            /// \return pointer to created record

            const ESM::ItemLevList *createOverrideRecord (const ESM::ItemLevList& record) override;
            ///< Write this record to the ESM store, allowing it to override a pre-existing record with the same ID.
            /// \return pointer to created record

            void update (float duration, bool paused) override;
            void updatePhysics (float duration, bool paused) override;

            void updateWindowManager () override;

            MWWorld::Ptr placeObject (const MWWorld::ConstPtr& object, float cursorX, float cursorY, int amount) override;
            ///< copy and place an object into the gameworld at the specified cursor position
            /// @param object
            /// @param cursor X (relative 0-1)
            /// @param cursor Y (relative 0-1)
            /// @param number of objects to place

            MWWorld::Ptr dropObjectOnGround (const MWWorld::Ptr& actor, const MWWorld::ConstPtr& object, int amount) override;
            ///< copy and place an object into the gameworld at the given actor's position
            /// @param actor giving the dropped object position
            /// @param object
            /// @param number of objects to place

            bool canPlaceObject(float cursorX, float cursorY) override;
            ///< @return true if it is possible to place on object at specified cursor location

            void processChangedSettings(const Settings::CategorySettingVector& settings) override;

            bool isFlying(const MWWorld::Ptr &ptr) const override;
            bool isSlowFalling(const MWWorld::Ptr &ptr) const override;
            ///Is the head of the creature underwater?
            bool isSubmerged(const MWWorld::ConstPtr &object) const override;
            bool isSwimming(const MWWorld::ConstPtr &object) const override;
            bool isUnderwater(const MWWorld::CellStore* cell, const osg::Vec3f &pos) const override;
            bool isUnderwater(const MWWorld::ConstPtr &object, const float heightRatio) const override;
            bool isWading(const MWWorld::ConstPtr &object) const override;
            bool isWaterWalkingCastableOnTarget(const MWWorld::ConstPtr &target) const override;
            bool isOnGround(const MWWorld::Ptr &ptr) const override;

            osg::Matrixf getActorHeadTransform(const MWWorld::ConstPtr& actor) const override;

            void togglePOV(bool force = false) override;

            bool isFirstPerson() const override;

            void togglePreviewMode(bool enable) override;

            bool toggleVanityMode(bool enable) override;

            void allowVanityMode(bool allow) override;

            void changeVanityModeScale(float factor) override;

            bool vanityRotateCamera(float * rot) override;
            void setCameraDistance(float dist, bool adjust = false, bool override = true) override;

            void setupPlayer() override;
            void renderPlayer() override;

            /// open or close a non-teleport door (depending on current state)
            void activateDoor(const MWWorld::Ptr& door) override;

            /// update movement state of a non-teleport door as specified
            /// @param state see MWClass::setDoorState
            /// @note throws an exception when invoked on a teleport door
            void activateDoor(const MWWorld::Ptr& door, MWWorld::DoorState state) override;

            void getActorsStandingOn (const MWWorld::ConstPtr& object, std::vector<MWWorld::Ptr> &actors) override; ///< get a list of actors standing on \a object
            bool getPlayerStandingOn (const MWWorld::ConstPtr& object) override; ///< @return true if the player is standing on \a object
            bool getActorStandingOn (const MWWorld::ConstPtr& object) override; ///< @return true if any actor is standing on \a object
            bool getPlayerCollidingWith(const MWWorld::ConstPtr& object) override; ///< @return true if the player is colliding with \a object
            bool getActorCollidingWith (const MWWorld::ConstPtr& object) override; ///< @return true if any actor is colliding with \a object
            void hurtStandingActors (const MWWorld::ConstPtr& object, float dmgPerSecond) override;
            ///< Apply a health difference to any actors standing on \a object.
            /// To hurt actors, healthPerSecond should be a positive value. For a negative value, actors will be healed.
            void hurtCollidingActors (const MWWorld::ConstPtr& object, float dmgPerSecond) override;
            ///< Apply a health difference to any actors colliding with \a object.
            /// To hurt actors, healthPerSecond should be a positive value. For a negative value, actors will be healed.

            float getWindSpeed() override;

            void getContainersOwnedBy (const MWWorld::ConstPtr& npc, std::vector<MWWorld::Ptr>& out) override;
            ///< get all containers in active cells owned by this Npc
            void getItemsOwnedBy (const MWWorld::ConstPtr& npc, std::vector<MWWorld::Ptr>& out) override;
            ///< get all items in active cells owned by this Npc

            bool getLOS(const MWWorld::ConstPtr& actor,const MWWorld::ConstPtr& targetActor) override;
            ///< get Line of Sight (morrowind stupid implementation)

            float getDistToNearestRayHit(const osg::Vec3f& from, const osg::Vec3f& dir, float maxDist, bool includeWater = false) override;

            void enableActorCollision(const MWWorld::Ptr& actor, bool enable) override;

            RestPermitted canRest() const override;
            ///< check if the player is allowed to rest

            void rest(double hours) override;
            void rechargeItems(double duration, bool activeOnly) override;

            /// \todo Probably shouldn't be here
            MWRender::Animation* getAnimation(const MWWorld::Ptr &ptr) override;
            const MWRender::Animation* getAnimation(const MWWorld::ConstPtr &ptr) const override;
            void reattachPlayerCamera() override;

            /// \todo this does not belong here
            void screenshot (osg::Image* image, int w, int h) override;
            bool screenshot360 (osg::Image* image, std::string settingStr) override;

            /// Find center of exterior cell above land surface
            /// \return false if exterior with given name not exists, true otherwise
            bool findExteriorPosition(const std::string &name, ESM::Position &pos) override;

            /// Find position in interior cell near door entrance
            /// \return false if interior with given name not exists, true otherwise
            bool findInteriorPosition(const std::string &name, ESM::Position &pos) override;

            /// Enables or disables use of teleport spell effects (recall, intervention, etc).
            void enableTeleporting(bool enable) override;

            /// Returns true if teleport spell effects are allowed.
            bool isTeleportingEnabled() const override;

            /// Enables or disables use of levitation spell effect.
            void enableLevitation(bool enable) override;

            /// Returns true if levitation spell effect is allowed.
            bool isLevitationEnabled() const override;

            bool getGodModeState() override;

            bool toggleGodMode() override;

            bool toggleScripts() override;
            bool getScriptsEnabled() const override;

            /**
             * @brief startSpellCast attempt to start casting a spell. Might fail immediately if conditions are not met.
             * @param actor
             * @return true if the spell can be casted (i.e. the animation should start)
             */
            bool startSpellCast (const MWWorld::Ptr& actor) override;

            /**
             * @brief Cast the actual spell, should be called mid-animation
             * @param actor
             */
            void castSpell (const MWWorld::Ptr& actor, bool manualSpell=false) override;

            void launchMagicBolt (const std::string& spellId, const MWWorld::Ptr& caster, const osg::Vec3f& fallbackDirection) override;
            void launchProjectile (MWWorld::Ptr& actor, MWWorld::Ptr& projectile,
                                           const osg::Vec3f& worldPos, const osg::Quat& orient, MWWorld::Ptr& bow, float speed, float attackStrength) override;

            void applyLoopingParticles(const MWWorld::Ptr& ptr) override;

            const std::vector<std::string>& getContentFiles() const override;
            void breakInvisibility (const MWWorld::Ptr& actor) override;

            // Allow NPCs to use torches?
            bool useTorches() const override;

            bool findInteriorPositionInWorldSpace(const MWWorld::CellStore* cell, osg::Vec3f& result) override;

            /// Teleports \a ptr to the closest reference of \a id (e.g. DivineMarker, PrisonMarker, TempleMarker)
            /// @note id must be lower case
            void teleportToClosestMarker (const MWWorld::Ptr& ptr,
                                                  const std::string& id) override;

            /// List all references (filtered by \a type) detected by \a ptr. The range
            /// is determined by the current magnitude of the "Detect X" magic effect belonging to \a type.
            /// @note This also works for references in containers.
            void listDetectedReferences (const MWWorld::Ptr& ptr, std::vector<MWWorld::Ptr>& out,
                                                  DetectionType type) override;

            /// Update the value of some globals according to the world state, which may be used by dialogue entries.
            /// This should be called when initiating a dialogue.
            void updateDialogueGlobals() override;

            /// Moves all stolen items from \a ptr to the closest evidence chest.
            void confiscateStolenItems(const MWWorld::Ptr& ptr) override;

            void goToJail () override;

            /// Spawn a random creature from a levelled list next to the player
            void spawnRandomCreature(const std::string& creatureList) override;

            /// Spawn a blood effect for \a ptr at \a worldPosition
            void spawnBloodEffect (const MWWorld::Ptr& ptr, const osg::Vec3f& worldPosition) override;

            void spawnEffect (const std::string& model, const std::string& textureOverride, const osg::Vec3f& worldPos, float scale = 1.f, bool isMagicVFX = true) override;

            void explodeSpell(const osg::Vec3f& origin, const ESM::EffectList& effects, const MWWorld::Ptr& caster, const MWWorld::Ptr& ignore,
                                      ESM::RangeType rangeType, const std::string& id, const std::string& sourceName,
                                      const bool fromProjectile=false) override;

            void activate (const MWWorld::Ptr& object, const MWWorld::Ptr& actor) override;

            /// @see MWWorld::WeatherManager::isInStorm
            bool isInStorm() const override;

            /// @see MWWorld::WeatherManager::getStormDirection
            osg::Vec3f getStormDirection() const override;

            /// Resets all actors in the current active cells to their original location within that cell.
            void resetActors() override;

            bool isWalkingOnWater (const MWWorld::ConstPtr& actor) const override;

            /// Return a vector aiming the actor's weapon towards a target.
            /// @note The length of the vector is the distance between actor and target.
            osg::Vec3f aimToTarget(const MWWorld::ConstPtr& actor, const MWWorld::ConstPtr& target) override;

            /// Return the distance between actor's weapon and target's collision box.
            float getHitDistance(const MWWorld::ConstPtr& actor, const MWWorld::ConstPtr& target) override;

            bool isPlayerInJail() const override;

            void setPlayerTraveling(bool traveling) override;
            bool isPlayerTraveling() const override;

            /// Return terrain height at \a worldPos position.
            float getTerrainHeightAt(const osg::Vec3f& worldPos) const override;

            /// Return physical or rendering half extents of the given actor.
            osg::Vec3f getHalfExtents(const MWWorld::ConstPtr& actor, bool rendering=false) const override;

            /// Export scene graph to a file and return the filename.
            /// \param ptr object to export scene graph for (if empty, export entire scene graph)
            std::string exportSceneGraph(const MWWorld::Ptr& ptr) override;

            /// Preload VFX associated with this effect list
            void preloadEffects(const ESM::EffectList* effectList) override;

            DetourNavigator::Navigator* getNavigator() const override;

            void updateActorPath(const MWWorld::ConstPtr& actor, const std::deque<osg::Vec3f>& path,
                    const osg::Vec3f& halfExtents, const osg::Vec3f& start, const osg::Vec3f& end) const override;

            void removeActorPath(const MWWorld::ConstPtr& actor) const override;

            void setNavMeshNumberToRender(const std::size_t value) override;

            /// Return physical half extents of the given actor to be used in pathfinding
            osg::Vec3f getPathfindingHalfExtents(const MWWorld::ConstPtr& actor) const override;

            bool hasCollisionWithDoor(const MWWorld::ConstPtr& door, const osg::Vec3f& position, const osg::Vec3f& destination) const override;

            bool isAreaOccupiedByOtherActor(const osg::Vec3f& position, const float radius, const MWWorld::ConstPtr& ignore) const override;
    };
}

#endif
