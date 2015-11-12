#ifndef GAME_MWWORLD_WORLDIMP_H
#define GAME_MWWORLD_WORLDIMP_H

#include <boost/shared_ptr.hpp>

#include <osg/ref_ptr>

#include "ptr.hpp"
#include "scene.hpp"
#include "esmstore.hpp"
#include "cells.hpp"
#include "localscripts.hpp"
#include "timestamp.hpp"
#include "fallback.hpp"
#include "globals.hpp"

#include "../mwbase/world.hpp"

#include "contentloader.hpp"

#include <components/settings/settings.hpp>

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

namespace MWWorld
{
    class WeatherManager;
    class Player;
    class ProjectileManager;

    /// \brief The game world and its visual representation

    class World : public MWBase::World
    {
            Resource::ResourceSystem* mResourceSystem;

            MWWorld::Fallback mFallback;
            MWRender::RenderingManager* mRendering;

            MWWorld::WeatherManager* mWeatherManager;

            MWWorld::Scene *mWorldScene;
            MWWorld::Player *mPlayer;
            std::vector<ESM::ESMReader> mEsm;
            MWWorld::ESMStore mStore;
            LocalScripts mLocalScripts;
            MWWorld::Globals mGlobalVariables;
            MWPhysics::PhysicsSystem *mPhysics;
            bool mSky;

            Cells mCells;

            std::string mCurrentWorldSpace;

            boost::shared_ptr<ProjectileManager> mProjectileManager;

            bool mGodMode;
            bool mScriptsEnabled;
            std::vector<std::string> mContentFiles;

            // not implemented
            World (const World&);
            World& operator= (const World&);

            int mActivationDistanceOverride;

            std::string mStartupScript;

            std::map<MWWorld::Ptr, int> mDoorStates;
            ///< only holds doors that are currently moving. 1 = opening, 2 = closing

            std::string mStartCell;

            void updateWeather(float duration, bool paused = false);
            int getDaysPerMonth (int month) const;

            void rotateObjectImp (const Ptr& ptr, const osg::Vec3f& rot, bool adjust);

            Ptr moveObjectImp (const Ptr& ptr, float x, float y, float z);
            ///< @return an updated Ptr in case the Ptr's cell changes

            Ptr copyObjectToCell(const Ptr &ptr, CellStore* cell, ESM::Position pos, bool adjustPos=true);

            void updateSoundListener();
            void updateWindowManager ();
            void updatePlayer(bool paused);
            MWWorld::Ptr getFacedObject(float maxDistance, bool ignorePlayer=true);

            void removeContainerScripts(const Ptr& reference);
            void addContainerScripts(const Ptr& reference, CellStore* cell);
            void PCDropped (const Ptr& item);

            void processDoors(float duration);
            ///< Run physics simulation and modify \a world accordingly.

            void doPhysics(float duration);
            ///< Run physics simulation and modify \a world accordingly.

            void ensureNeededRecords();

            /**
             * @brief loadContentFiles - Loads content files (esm,esp,omwgame,omwaddon)
             * @param fileCollections- Container which holds content file names and their paths
             * @param content - Container which holds content file names
             * @param contentLoader -
             */
            void loadContentFiles(const Files::Collections& fileCollections,
                const std::vector<std::string>& content, ContentLoader& contentLoader);

            float mSwimHeightScale;
            bool isUnderwater(const MWWorld::Ptr &object, const float heightRatio) const;
            ///< helper function for implementing isSwimming(), isSubmerged(), isWading()

            bool mTeleportEnabled;
            bool mLevitationEnabled;
            bool mGoToJail;
            int mDaysInPrison;

            float feetToGameUnits(float feet);

            MWWorld::Ptr getClosestMarker( const MWWorld::Ptr &ptr, const std::string &id );
            MWWorld::Ptr getClosestMarkerFromExteriorPosition( const osg::Vec3f& worldPos, const std::string &id );

        public:

            World (
                osgViewer::Viewer* viewer,
                osg::ref_ptr<osg::Group> rootNode,
                Resource::ResourceSystem* resourceSystem,
                const Files::Collections& fileCollections,
                const std::vector<std::string>& contentFiles,
                ToUTF8::Utf8Encoder* encoder, const std::map<std::string,std::string>& fallbackMap,
                int activationDistanceOverride, const std::string& startCell, const std::string& startupScript, const std::string& resourcePath);

            virtual ~World();

            virtual void startNewGame (bool bypass);
            ///< \param bypass Bypass regular game start.

            virtual void clear();

            virtual int countSavedGameRecords() const;
            virtual int countSavedGameCells() const;

            virtual void write (ESM::ESMWriter& writer, Loading::Listener& progress) const;

            virtual void readRecord (ESM::ESMReader& reader, uint32_t type,
                const std::map<int, int>& contentFileMap);

            virtual CellStore *getExterior (int x, int y);

            virtual CellStore *getInterior (const std::string& name);

            virtual CellStore *getCell (const ESM::CellId& id);

            //switch to POV before showing player's death animation
            virtual void useDeathCamera();

            virtual void setWaterHeight(const float height);

            virtual bool toggleWater();
            virtual bool toggleWorld();

            virtual void adjustSky();

            virtual const Fallback *getFallback() const;

            virtual Player& getPlayer();
            virtual MWWorld::Ptr getPlayerPtr();

            virtual const MWWorld::ESMStore& getStore() const;

            virtual std::vector<ESM::ESMReader>& getEsmReader();

            virtual LocalScripts& getLocalScripts();

            virtual bool hasCellChanged() const;
            ///< Has the set of active cells changed, since the last frame?

            virtual bool isCellExterior() const;

            virtual bool isCellQuasiExterior() const;

            virtual osg::Vec2f getNorthVector (CellStore* cell);
            ///< get north vector for given interior cell

            virtual void getDoorMarkers (MWWorld::CellStore* cell, std::vector<DoorMarker>& out);
            ///< get a list of teleport door markers for a given cell, to be displayed on the local map

            virtual void setGlobalInt (const std::string& name, int value);
            ///< Set value independently from real type.

            virtual void setGlobalFloat (const std::string& name, float value);
            ///< Set value independently from real type.

            virtual int getGlobalInt (const std::string& name) const;
            ///< Get value independently from real type.

            virtual float getGlobalFloat (const std::string& name) const;
            ///< Get value independently from real type.

            virtual char getGlobalVariableType (const std::string& name) const;
            ///< Return ' ', if there is no global variable with this name.

            virtual std::string getCellName (const MWWorld::CellStore *cell = 0) const;
            ///< Return name of the cell.
            ///
            /// \note If cell==0, the cell the player is currently in will be used instead to
            /// generate a name.

            virtual void removeRefScript (MWWorld::RefData *ref);
            //< Remove the script attached to ref from mLocalScripts

            virtual Ptr getPtr (const std::string& name, bool activeOnly);
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            virtual Ptr searchPtr (const std::string& name, bool activeOnly);
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            virtual Ptr searchPtrViaActorId (int actorId);
            ///< Search is limited to the active cells.

            virtual MWWorld::Ptr findContainer (const MWWorld::Ptr& ptr);
            ///< Return a pointer to a liveCellRef which contains \a ptr.
            /// \note Search is limited to the active cells.

            virtual void adjustPosition (const Ptr& ptr, bool force);
            ///< Adjust position after load to be on ground. Must be called after model load.
            /// @param force do this even if the ptr is flying

            virtual void fixPosition (const Ptr& actor);
            ///< Attempt to fix position so that the Ptr is no longer inside collision geometry.

            virtual void enable (const Ptr& ptr);

            virtual void disable (const Ptr& ptr);

            virtual void advanceTime (double hours, bool incremental = false);
            ///< Advance in-game time.

            virtual void setHour (double hour);
            ///< Set in-game time hour.

            virtual void setMonth (int month);
            ///< Set in-game time month.

            virtual void setDay (int day);
            ///< Set in-game time day.

            virtual int getDay() const;
            virtual int getMonth() const;
            virtual int getYear() const;

            virtual std::string getMonthName (int month = -1) const;
            ///< Return name of month (-1: current month)

            virtual TimeStamp getTimeStamp() const;
            ///< Return current in-game time stamp.

            virtual bool toggleSky();
            ///< \return Resulting mode

            virtual void changeWeather (const std::string& region, unsigned int id);

            virtual int getCurrentWeather() const;

            virtual int getMasserPhase() const;

            virtual int getSecundaPhase() const;

            virtual void setMoonColour (bool red);

            virtual void modRegion(const std::string &regionid, const std::vector<char> &chances);

            virtual float getTimeScaleFactor() const;

            virtual void changeToInteriorCell (const std::string& cellName,
                const ESM::Position& position);
            ///< Move to interior cell.

            virtual void changeToExteriorCell (const ESM::Position& position);
            ///< Move to exterior cell.

            virtual void changeToCell (const ESM::CellId& cellId, const ESM::Position& position, bool detectWorldSpaceChange=true);
            ///< @param detectWorldSpaceChange if true, clean up worldspace-specific data when the world space changes

            virtual const ESM::Cell *getExterior (const std::string& cellName) const;
            ///< Return a cell matching the given name or a 0-pointer, if there is no such cell.

            virtual void markCellAsUnchanged();

            virtual MWWorld::Ptr getFacedObject();
            ///< Return pointer to the object the player is looking at, if it is within activation range

            /// Returns a pointer to the object the provided object would hit (if within the
            /// specified distance), and the point where the hit occurs. This will attempt to
            /// use the "Head" node as a basis.
            virtual std::pair<MWWorld::Ptr,osg::Vec3f> getHitContact(const MWWorld::Ptr &ptr, float distance);

            /// @note No-op for items in containers. Use ContainerStore::removeItem instead.
            virtual void deleteObject (const Ptr& ptr);

            virtual void undeleteObject (const Ptr& ptr);

            virtual MWWorld::Ptr moveObject (const Ptr& ptr, float x, float y, float z);
            ///< @return an updated Ptr in case the Ptr's cell changes

            virtual MWWorld::Ptr moveObject (const Ptr& ptr, CellStore* newCell, float x, float y, float z);
            ///< @return an updated Ptr

            virtual void scaleObject (const Ptr& ptr, float scale);

            /// World rotates object, uses radians
            /// @note Rotations via this method use a different rotation order than the initial rotations in the CS. This
            /// could be considered a bug, but is needed for MW compatibility.
            /// \param adjust indicates rotation should be set or adjusted
            virtual void rotateObject (const Ptr& ptr,float x,float y,float z, bool adjust = false);

            virtual MWWorld::Ptr safePlaceObject(const MWWorld::Ptr& ptr, MWWorld::CellStore* cell, ESM::Position pos);
            ///< place an object in a "safe" location (ie not in the void, etc). Makes a copy of the Ptr.

            virtual float getMaxActivationDistance();

            virtual void indexToPosition (int cellX, int cellY, float &x, float &y, bool centre = false)
                const;
            ///< Convert cell numbers to position.

            virtual void positionToIndex (float x, float y, int &cellX, int &cellY) const;
            ///< Convert position to cell numbers

            virtual void queueMovement(const Ptr &ptr, const osg::Vec3f &velocity);
            ///< Queues movement for \a ptr (in local space), to be applied in the next call to
            /// doPhysics.

            virtual bool castRay (float x1, float y1, float z1, float x2, float y2, float z2);
            ///< cast a Ray and return true if there is an object in the ray path.

            virtual bool toggleCollisionMode();
            ///< Toggle collision mode for player. If disabled player object should ignore
            /// collisions and gravity.
            ///< \return Resulting mode

            virtual bool toggleRenderMode (MWRender::RenderMode mode);
            ///< Toggle a render mode.
            ///< \return Resulting mode

            virtual const ESM::Potion *createRecord (const ESM::Potion& record);
            ///< Create a new record (of type potion) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Spell *createRecord (const ESM::Spell& record);
            ///< Create a new record (of type spell) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Class *createRecord (const ESM::Class& record);
            ///< Create a new record (of type class) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Cell *createRecord (const ESM::Cell& record);
            ///< Create a new record (of type cell) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::NPC *createRecord(const ESM::NPC &record);
            ///< Create a new record (of type npc) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Armor *createRecord (const ESM::Armor& record);
            ///< Create a new record (of type armor) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Weapon *createRecord (const ESM::Weapon& record);
            ///< Create a new record (of type weapon) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Clothing *createRecord (const ESM::Clothing& record);
            ///< Create a new record (of type clothing) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Enchantment *createRecord (const ESM::Enchantment& record);
            ///< Create a new record (of type enchantment) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Book *createRecord (const ESM::Book& record);
            ///< Create a new record (of type book) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::CreatureLevList *createOverrideRecord (const ESM::CreatureLevList& record);
            ///< Write this record to the ESM store, allowing it to override a pre-existing record with the same ID.
            /// \return pointer to created record

            virtual const ESM::ItemLevList *createOverrideRecord (const ESM::ItemLevList& record);
            ///< Write this record to the ESM store, allowing it to override a pre-existing record with the same ID.
            /// \return pointer to created record

            virtual void update (float duration, bool paused);

            virtual MWWorld::Ptr placeObject (const MWWorld::Ptr& object, float cursorX, float cursorY, int amount);
            ///< copy and place an object into the gameworld at the specified cursor position
            /// @param object
            /// @param cursor X (relative 0-1)
            /// @param cursor Y (relative 0-1)
            /// @param number of objects to place

            virtual MWWorld::Ptr dropObjectOnGround (const MWWorld::Ptr& actor, const MWWorld::Ptr& object, int amount);
            ///< copy and place an object into the gameworld at the given actor's position
            /// @param actor giving the dropped object position
            /// @param object
            /// @param number of objects to place

            virtual bool canPlaceObject(float cursorX, float cursorY);
            ///< @return true if it is possible to place on object at specified cursor location

            virtual void processChangedSettings(const Settings::CategorySettingVector& settings);

            virtual bool isFlying(const MWWorld::Ptr &ptr) const;
            virtual bool isSlowFalling(const MWWorld::Ptr &ptr) const;
            ///Is the head of the creature underwater?
            virtual bool isSubmerged(const MWWorld::Ptr &object) const;
            virtual bool isSwimming(const MWWorld::Ptr &object) const;
            virtual bool isUnderwater(const MWWorld::CellStore* cell, const osg::Vec3f &pos) const;
            virtual bool isWading(const MWWorld::Ptr &object) const;
            virtual bool isOnGround(const MWWorld::Ptr &ptr) const;

            virtual void togglePOV();

            virtual bool isFirstPerson() const;

            virtual void togglePreviewMode(bool enable);

            virtual bool toggleVanityMode(bool enable);

            virtual void allowVanityMode(bool allow);

            virtual void togglePlayerLooking(bool enable);

            virtual void changeVanityModeScale(float factor);

            virtual bool vanityRotateCamera(float * rot);
            virtual void setCameraDistance(float dist, bool adjust = false, bool override = true);

            virtual void setupPlayer();
            virtual void renderPlayer();

            /// open or close a non-teleport door (depending on current state)
            virtual void activateDoor(const MWWorld::Ptr& door);

            /// update movement state of a non-teleport door as specified
            /// @param state see MWClass::setDoorState
            /// @note throws an exception when invoked on a teleport door
            virtual void activateDoor(const MWWorld::Ptr& door, int state);

            virtual bool getPlayerStandingOn (const MWWorld::Ptr& object); ///< @return true if the player is standing on \a object
            virtual bool getActorStandingOn (const MWWorld::Ptr& object); ///< @return true if any actor is standing on \a object
            virtual bool getPlayerCollidingWith(const MWWorld::Ptr& object); ///< @return true if the player is colliding with \a object
            virtual bool getActorCollidingWith (const MWWorld::Ptr& object); ///< @return true if any actor is colliding with \a object
            virtual void hurtStandingActors (const MWWorld::Ptr& object, float dmgPerSecond);
            ///< Apply a health difference to any actors standing on \a object.
            /// To hurt actors, healthPerSecond should be a positive value. For a negative value, actors will be healed.
            virtual void hurtCollidingActors (const MWWorld::Ptr& object, float dmgPerSecond);
            ///< Apply a health difference to any actors colliding with \a object.
            /// To hurt actors, healthPerSecond should be a positive value. For a negative value, actors will be healed.

            virtual float getWindSpeed();

            virtual void getContainersOwnedBy (const MWWorld::Ptr& npc, std::vector<MWWorld::Ptr>& out);
            ///< get all containers in active cells owned by this Npc
            virtual void getItemsOwnedBy (const MWWorld::Ptr& npc, std::vector<MWWorld::Ptr>& out);
            ///< get all items in active cells owned by this Npc

            virtual bool getLOS(const MWWorld::Ptr& actor,const MWWorld::Ptr& targetActor);
            ///< get Line of Sight (morrowind stupid implementation)

            virtual float getDistToNearestRayHit(const osg::Vec3f& from, const osg::Vec3f& dir, float maxDist);

            virtual void enableActorCollision(const MWWorld::Ptr& actor, bool enable);

            virtual int canRest();
            ///< check if the player is allowed to rest \n
            /// 0 - yes \n
            /// 1 - only waiting \n
            /// 2 - player is underwater \n
            /// 3 - enemies are nearby (not implemented)

            /// \todo Probably shouldn't be here
            virtual MWRender::Animation* getAnimation(const MWWorld::Ptr &ptr);
            virtual void reattachPlayerCamera();

            /// \todo this does not belong here
            virtual void screenshot (osg::Image* image, int w, int h);

            /// Find center of exterior cell above land surface
            /// \return false if exterior with given name not exists, true otherwise
            virtual bool findExteriorPosition(const std::string &name, ESM::Position &pos);

            /// Find position in interior cell near door entrance
            /// \return false if interior with given name not exists, true otherwise
            virtual bool findInteriorPosition(const std::string &name, ESM::Position &pos);

            /// Enables or disables use of teleport spell effects (recall, intervention, etc).
            virtual void enableTeleporting(bool enable);

            /// Returns true if teleport spell effects are allowed.
            virtual bool isTeleportingEnabled() const;

            /// Enables or disables use of levitation spell effect.
            virtual void enableLevitation(bool enable);

            /// Returns true if levitation spell effect is allowed.
            virtual bool isLevitationEnabled() const;

            virtual void setWerewolf(const MWWorld::Ptr& actor, bool werewolf);

            virtual void applyWerewolfAcrobatics(const MWWorld::Ptr& actor);

            virtual bool getGodModeState();

            virtual bool toggleGodMode();

            virtual bool toggleScripts();
            virtual bool getScriptsEnabled() const;

            /**
             * @brief startSpellCast attempt to start casting a spell. Might fail immediately if conditions are not met.
             * @param actor
             * @return true if the spell can be casted (i.e. the animation should start)
             */
            virtual bool startSpellCast (const MWWorld::Ptr& actor);

            /**
             * @brief Cast the actual spell, should be called mid-animation
             * @param actor
             */
            virtual void castSpell (const MWWorld::Ptr& actor);

            virtual void launchMagicBolt (const std::string& model, const std::string& sound, const std::string& spellId,
                                          float speed, bool stack, const ESM::EffectList& effects,
                                           const MWWorld::Ptr& caster, const std::string& sourceName, const osg::Vec3f& fallbackDirection);
            virtual void launchProjectile (MWWorld::Ptr actor, MWWorld::Ptr projectile,
                                           const osg::Vec3f& worldPos, const osg::Quat& orient, MWWorld::Ptr bow, float speed, float attackStrength);


            virtual const std::vector<std::string>& getContentFiles() const;

            virtual void breakInvisibility (const MWWorld::Ptr& actor);
            // Are we in an exterior or pseudo-exterior cell and it's night?
            virtual bool isDark() const;

            virtual bool findInteriorPositionInWorldSpace(MWWorld::CellStore* cell, osg::Vec3f& result);

            /// Teleports \a ptr to the closest reference of \a id (e.g. DivineMarker, PrisonMarker, TempleMarker)
            /// @note id must be lower case
            virtual void teleportToClosestMarker (const MWWorld::Ptr& ptr,
                                                  const std::string& id);

            /// List all references (filtered by \a type) detected by \a ptr. The range
            /// is determined by the current magnitude of the "Detect X" magic effect belonging to \a type.
            /// @note This also works for references in containers.
            virtual void listDetectedReferences (const MWWorld::Ptr& ptr, std::vector<MWWorld::Ptr>& out,
                                                  DetectionType type);

            /// Update the value of some globals according to the world state, which may be used by dialogue entries.
            /// This should be called when initiating a dialogue.
            virtual void updateDialogueGlobals();

            /// Moves all stolen items from \a ptr to the closest evidence chest.
            virtual void confiscateStolenItems(const MWWorld::Ptr& ptr);

            virtual void goToJail ();

            /// Spawn a random creature from a levelled list next to the player
            virtual void spawnRandomCreature(const std::string& creatureList);

            /// Spawn a blood effect for \a ptr at \a worldPosition
            virtual void spawnBloodEffect (const MWWorld::Ptr& ptr, const osg::Vec3f& worldPosition);

            virtual void spawnEffect (const std::string& model, const std::string& textureOverride, const osg::Vec3f& worldPos);

            virtual void explodeSpell (const osg::Vec3f& origin, const ESM::EffectList& effects,
                                       const MWWorld::Ptr& caster, ESM::RangeType rangeType, const std::string& id, const std::string& sourceName);

            virtual void activate (const MWWorld::Ptr& object, const MWWorld::Ptr& actor);

            /// @see MWWorld::WeatherManager::isInStorm
            virtual bool isInStorm() const;

            /// @see MWWorld::WeatherManager::getStormDirection
            virtual osg::Vec3f getStormDirection() const;

            /// Resets all actors in the current active cells to their original location within that cell.
            virtual void resetActors();

            virtual bool isWalkingOnWater (const MWWorld::Ptr& actor);

            /// Return a vector aiming the actor's weapon towards a target.
            /// @note The length of the vector is the distance between actor and target.
            virtual osg::Vec3f aimToTarget(const MWWorld::Ptr& actor, const MWWorld::Ptr& target);
    };
}

#endif
