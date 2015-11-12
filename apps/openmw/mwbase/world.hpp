#ifndef GAME_MWBASE_WORLD_H
#define GAME_MWBASE_WORLD_H

#include <vector>
#include <map>
#include <set>

#include <components/esm/cellid.hpp>

#include "../mwworld/ptr.hpp"

#include "../mwrender/rendermode.hpp"

namespace osg
{
    class Vec3f;
    class Quat;
    class Image;
}

namespace Loading
{
    class Listener;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
    struct Position;
    struct Cell;
    struct Class;
    struct Potion;
    struct Spell;
    struct NPC;
    struct Armor;
    struct Weapon;
    struct Clothing;
    struct Enchantment;
    struct Book;
    struct EffectList;
    struct CreatureLevList;
    struct ItemLevList;
}

namespace MWRender
{
    class Animation;
}

namespace MWMechanics
{
    struct Movement;
}

namespace MWWorld
{
    class Fallback;
    class CellStore;
    class Player;
    class LocalScripts;
    class TimeStamp;
    class ESMStore;
    class RefData;

    typedef std::vector<std::pair<MWWorld::Ptr,MWMechanics::Movement> > PtrMovementList;
}

namespace MWBase
{
    /// \brief Interface for the World (implemented in MWWorld)
    class World
    {
            World (const World&);
            ///< not implemented

            World& operator= (const World&);
            ///< not implemented

        public:

            struct DoorMarker
            {
                std::string name;
                float x, y; // world position
                ESM::CellId dest;
            };

            World() {}

            virtual ~World() {}

            virtual void startNewGame (bool bypass) = 0;
            ///< \param bypass Bypass regular game start.

            virtual void clear() = 0;

            virtual int countSavedGameRecords() const = 0;
            virtual int countSavedGameCells() const = 0;

            virtual void write (ESM::ESMWriter& writer, Loading::Listener& listener) const = 0;

            virtual void readRecord (ESM::ESMReader& reader, uint32_t type,
                const std::map<int, int>& contentFileMap) = 0;

            virtual MWWorld::CellStore *getExterior (int x, int y) = 0;

            virtual MWWorld::CellStore *getInterior (const std::string& name) = 0;

            virtual MWWorld::CellStore *getCell (const ESM::CellId& id) = 0;

            virtual void useDeathCamera() = 0;

            virtual void setWaterHeight(const float height) = 0;

            virtual bool toggleWater() = 0;
            virtual bool toggleWorld() = 0;

            virtual void adjustSky() = 0;

            virtual const MWWorld::Fallback *getFallback () const = 0;

            virtual MWWorld::Player& getPlayer() = 0;
            virtual MWWorld::Ptr getPlayerPtr() = 0;

            virtual const MWWorld::ESMStore& getStore() const = 0;

            virtual std::vector<ESM::ESMReader>& getEsmReader() = 0;

            virtual MWWorld::LocalScripts& getLocalScripts() = 0;

            virtual bool hasCellChanged() const = 0;
            ///< Has the set of active cells changed, since the last frame?

            virtual bool isCellExterior() const = 0;

            virtual bool isCellQuasiExterior() const = 0;

            virtual osg::Vec2f getNorthVector (MWWorld::CellStore* cell) = 0;
            ///< get north vector for given interior cell

            virtual void getDoorMarkers (MWWorld::CellStore* cell, std::vector<DoorMarker>& out) = 0;
            ///< get a list of teleport door markers for a given cell, to be displayed on the local map

            virtual void setGlobalInt (const std::string& name, int value) = 0;
            ///< Set value independently from real type.

            virtual void setGlobalFloat (const std::string& name, float value) = 0;
            ///< Set value independently from real type.

            virtual int getGlobalInt (const std::string& name) const = 0;
            ///< Get value independently from real type.

            virtual float getGlobalFloat (const std::string& name) const = 0;
            ///< Get value independently from real type.

            virtual char getGlobalVariableType (const std::string& name) const = 0;
            ///< Return ' ', if there is no global variable with this name.

            virtual std::string getCellName (const MWWorld::CellStore *cell = 0) const = 0;
            ///< Return name of the cell.
            ///
            /// \note If cell==0, the cell the player is currently in will be used instead to
            /// generate a name.

            virtual void removeRefScript (MWWorld::RefData *ref) = 0;
            //< Remove the script attached to ref from mLocalScripts

            virtual MWWorld::Ptr getPtr (const std::string& name, bool activeOnly) = 0;
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            virtual MWWorld::Ptr searchPtr (const std::string& name, bool activeOnly) = 0;
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            virtual MWWorld::Ptr searchPtrViaActorId (int actorId) = 0;
            ///< Search is limited to the active cells.

            virtual MWWorld::Ptr findContainer (const MWWorld::Ptr& ptr) = 0;
            ///< Return a pointer to a liveCellRef which contains \a ptr.
            /// \note Search is limited to the active cells.

            virtual void enable (const MWWorld::Ptr& ptr) = 0;

            virtual void disable (const MWWorld::Ptr& ptr) = 0;

            virtual void advanceTime (double hours, bool incremental = false) = 0;
            ///< Advance in-game time.

            virtual void setHour (double hour) = 0;
            ///< Set in-game time hour.

            virtual void setMonth (int month) = 0;
            ///< Set in-game time month.

            virtual void setDay (int day) = 0;
            ///< Set in-game time day.

            virtual int getDay() const = 0;
            virtual int getMonth() const = 0;
            virtual int getYear() const = 0;

            virtual std::string getMonthName (int month = -1) const = 0;
            ///< Return name of month (-1: current month)

            virtual MWWorld::TimeStamp getTimeStamp() const = 0;
            ///< Return current in-game time stamp.

            virtual bool toggleSky() = 0;
            ///< \return Resulting mode

            virtual void changeWeather(const std::string& region, unsigned int id) = 0;

            virtual int getCurrentWeather() const = 0;

            virtual int getMasserPhase() const = 0;

            virtual int getSecundaPhase() const = 0;

            virtual void setMoonColour (bool red) = 0;

            virtual void modRegion(const std::string &regionid, const std::vector<char> &chances) = 0;

            virtual float getTimeScaleFactor() const = 0;

            virtual void changeToInteriorCell (const std::string& cellName,
                const ESM::Position& position) = 0;
            ///< Move to interior cell.

            virtual void changeToExteriorCell (const ESM::Position& position) = 0;
            ///< Move to exterior cell.

            virtual void changeToCell (const ESM::CellId& cellId, const ESM::Position& position, bool detectWorldSpaceChange=true) = 0;
            ///< @param detectWorldSpaceChange if true, clean up worldspace-specific data when the world space changes

            virtual const ESM::Cell *getExterior (const std::string& cellName) const = 0;
            ///< Return a cell matching the given name or a 0-pointer, if there is no such cell.

            virtual void markCellAsUnchanged() = 0;

            virtual MWWorld::Ptr  getFacedObject() = 0;
            ///< Return pointer to the object the player is looking at, if it is within activation range

            virtual float getMaxActivationDistance() = 0;

            /// Returns a pointer to the object the provided object would hit (if within the
            /// specified distance), and the point where the hit occurs. This will attempt to
            /// use the "Head" node, or alternatively the "Bip01 Head" node as a basis.
            virtual std::pair<MWWorld::Ptr,osg::Vec3f> getHitContact(const MWWorld::Ptr &ptr, float distance) = 0;

            virtual void adjustPosition (const MWWorld::Ptr& ptr, bool force) = 0;
            ///< Adjust position after load to be on ground. Must be called after model load.
            /// @param force do this even if the ptr is flying

            virtual void fixPosition (const MWWorld::Ptr& actor) = 0;
            ///< Attempt to fix position so that the Ptr is no longer inside collision geometry.

            /// @note No-op for items in containers. Use ContainerStore::removeItem instead.
            virtual void deleteObject (const MWWorld::Ptr& ptr) = 0;
            virtual void undeleteObject (const MWWorld::Ptr& ptr) = 0;

            virtual MWWorld::Ptr moveObject (const MWWorld::Ptr& ptr, float x, float y, float z) = 0;
            ///< @return an updated Ptr in case the Ptr's cell changes

            virtual MWWorld::Ptr moveObject(const MWWorld::Ptr &ptr, MWWorld::CellStore* newCell, float x, float y, float z) = 0;
            ///< @return an updated Ptr

            virtual void scaleObject (const MWWorld::Ptr& ptr, float scale) = 0;

            virtual void rotateObject(const MWWorld::Ptr& ptr,float x,float y,float z, bool adjust = false) = 0;

            virtual MWWorld::Ptr safePlaceObject(const MWWorld::Ptr& ptr, MWWorld::CellStore* cell, ESM::Position pos) = 0;
            ///< place an object in a "safe" location (ie not in the void, etc).

            virtual void indexToPosition (int cellX, int cellY, float &x, float &y, bool centre = false)
                const = 0;
            ///< Convert cell numbers to position.

            virtual void positionToIndex (float x, float y, int &cellX, int &cellY) const = 0;
            ///< Convert position to cell numbers

            virtual void queueMovement(const MWWorld::Ptr &ptr, const osg::Vec3f &velocity) = 0;
            ///< Queues movement for \a ptr (in local space), to be applied in the next call to
            /// doPhysics.

            virtual bool castRay (float x1, float y1, float z1, float x2, float y2, float z2) = 0;
            ///< cast a Ray and return true if there is an object in the ray path.

            virtual bool toggleCollisionMode() = 0;
            ///< Toggle collision mode for player. If disabled player object should ignore
            /// collisions and gravity.
            /// \return Resulting mode

            virtual bool toggleRenderMode (MWRender::RenderMode mode) = 0;
            ///< Toggle a render mode.
            ///< \return Resulting mode

            virtual const ESM::Potion *createRecord (const ESM::Potion& record) = 0;
            ///< Create a new record (of type potion) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Spell *createRecord (const ESM::Spell& record) = 0;
            ///< Create a new record (of type spell) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Class *createRecord (const ESM::Class& record) = 0;
            ///< Create a new record (of type class) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Cell *createRecord (const ESM::Cell& record) = 0;
            ///< Create a new record (of type cell) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::NPC *createRecord(const ESM::NPC &record) = 0;
            ///< Create a new record (of type npc) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Armor *createRecord (const ESM::Armor& record) = 0;
            ///< Create a new record (of type armor) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Weapon *createRecord (const ESM::Weapon& record) = 0;
            ///< Create a new record (of type weapon) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Clothing *createRecord (const ESM::Clothing& record) = 0;
            ///< Create a new record (of type clothing) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Enchantment *createRecord (const ESM::Enchantment& record) = 0;
            ///< Create a new record (of type enchantment) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Book *createRecord (const ESM::Book& record) = 0;
            ///< Create a new record (of type book) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::CreatureLevList *createOverrideRecord (const ESM::CreatureLevList& record) = 0;
            ///< Write this record to the ESM store, allowing it to override a pre-existing record with the same ID.
            /// \return pointer to created record

            virtual const ESM::ItemLevList *createOverrideRecord (const ESM::ItemLevList& record) = 0;
            ///< Write this record to the ESM store, allowing it to override a pre-existing record with the same ID.
            /// \return pointer to created record

            virtual void update (float duration, bool paused) = 0;

            virtual MWWorld::Ptr placeObject (const MWWorld::Ptr& object, float cursorX, float cursorY, int amount) = 0;
            ///< copy and place an object into the gameworld at the specified cursor position
            /// @param object
            /// @param cursor X (relative 0-1)
            /// @param cursor Y (relative 0-1)
            /// @param number of objects to place

            virtual MWWorld::Ptr dropObjectOnGround (const MWWorld::Ptr& actor, const MWWorld::Ptr& object, int amount) = 0;
            ///< copy and place an object into the gameworld at the given actor's position
            /// @param actor giving the dropped object position
            /// @param object
            /// @param number of objects to place

            virtual bool canPlaceObject (float cursorX, float cursorY) = 0;
            ///< @return true if it is possible to place on object at specified cursor location

            virtual void processChangedSettings (const std::set< std::pair<std::string, std::string> >& settings) = 0;

            virtual bool isFlying(const MWWorld::Ptr &ptr) const = 0;
            virtual bool isSlowFalling(const MWWorld::Ptr &ptr) const = 0;
            virtual bool isSwimming(const MWWorld::Ptr &object) const = 0;
            virtual bool isWading(const MWWorld::Ptr &object) const = 0;
            ///Is the head of the creature underwater?
            virtual bool isSubmerged(const MWWorld::Ptr &object) const = 0;
            virtual bool isUnderwater(const MWWorld::CellStore* cell, const osg::Vec3f &pos) const = 0;
            virtual bool isOnGround(const MWWorld::Ptr &ptr) const = 0;

            virtual void togglePOV() = 0;
            virtual bool isFirstPerson() const = 0;
            virtual void togglePreviewMode(bool enable) = 0;
            virtual bool toggleVanityMode(bool enable) = 0;
            virtual void allowVanityMode(bool allow) = 0;
            virtual void togglePlayerLooking(bool enable) = 0;
            virtual void changeVanityModeScale(float factor) = 0;
            virtual bool vanityRotateCamera(float * rot) = 0;
            virtual void setCameraDistance(float dist, bool adjust = false, bool override = true)=0;

            virtual void setupPlayer() = 0;
            virtual void renderPlayer() = 0;

            /// open or close a non-teleport door (depending on current state)
            virtual void activateDoor(const MWWorld::Ptr& door) = 0;
            /// update movement state of a non-teleport door as specified
            /// @param state see MWClass::setDoorState
            /// @note throws an exception when invoked on a teleport door
            virtual void activateDoor(const MWWorld::Ptr& door, int state) = 0;

            virtual bool getPlayerStandingOn (const MWWorld::Ptr& object) = 0; ///< @return true if the player is standing on \a object
            virtual bool getActorStandingOn (const MWWorld::Ptr& object) = 0; ///< @return true if any actor is standing on \a object
            virtual bool getPlayerCollidingWith(const MWWorld::Ptr& object) = 0; ///< @return true if the player is colliding with \a object
            virtual bool getActorCollidingWith (const MWWorld::Ptr& object) = 0; ///< @return true if any actor is colliding with \a object
            virtual void hurtStandingActors (const MWWorld::Ptr& object, float dmgPerSecond) = 0;
            ///< Apply a health difference to any actors standing on \a object.
            /// To hurt actors, healthPerSecond should be a positive value. For a negative value, actors will be healed.
            virtual void hurtCollidingActors (const MWWorld::Ptr& object, float dmgPerSecond) = 0;
            ///< Apply a health difference to any actors colliding with \a object.
            /// To hurt actors, healthPerSecond should be a positive value. For a negative value, actors will be healed.

            virtual float getWindSpeed() = 0;

            virtual void getContainersOwnedBy (const MWWorld::Ptr& npc, std::vector<MWWorld::Ptr>& out) = 0;
            ///< get all containers in active cells owned by this Npc
            virtual void getItemsOwnedBy (const MWWorld::Ptr& npc, std::vector<MWWorld::Ptr>& out) = 0;
            ///< get all items in active cells owned by this Npc

            virtual bool getLOS(const MWWorld::Ptr& actor,const MWWorld::Ptr& targetActor) = 0;
            ///< get Line of Sight (morrowind stupid implementation)

            virtual float getDistToNearestRayHit(const osg::Vec3f& from, const osg::Vec3f& dir, float maxDist) = 0;

            virtual void enableActorCollision(const MWWorld::Ptr& actor, bool enable) = 0;

            virtual int canRest() = 0;
            ///< check if the player is allowed to rest \n
            /// 0 - yes \n
            /// 1 - only waiting \n
            /// 2 - player is underwater \n
            /// 3 - enemies are nearby (not implemented)

            /// \todo Probably shouldn't be here
            virtual MWRender::Animation* getAnimation(const MWWorld::Ptr &ptr) = 0;
            virtual void reattachPlayerCamera() = 0;

            /// \todo this does not belong here
            virtual void screenshot (osg::Image* image, int w, int h) = 0;

            /// Find default position inside exterior cell specified by name
            /// \return false if exterior with given name not exists, true otherwise
            virtual bool findExteriorPosition(const std::string &name, ESM::Position &pos) = 0;

            /// Find default position inside interior cell specified by name
            /// \return false if interior with given name not exists, true otherwise
            virtual bool findInteriorPosition(const std::string &name, ESM::Position &pos) = 0;

            /// Enables or disables use of teleport spell effects (recall, intervention, etc).
            virtual void enableTeleporting(bool enable) = 0;

            /// Returns true if teleport spell effects are allowed.
            virtual bool isTeleportingEnabled() const = 0;

            /// Enables or disables use of levitation spell effect.
            virtual void enableLevitation(bool enable) = 0;

            /// Returns true if levitation spell effect is allowed.
            virtual bool isLevitationEnabled() const = 0;

            /// Turn actor into werewolf or normal form.
            virtual void setWerewolf(const MWWorld::Ptr& actor, bool werewolf) = 0;

            /// Sets the NPC's Acrobatics skill to match the fWerewolfAcrobatics GMST.
            /// It only applies to the current form the NPC is in.
            virtual void applyWerewolfAcrobatics(const MWWorld::Ptr& actor) = 0;

            virtual bool getGodModeState() = 0;

            virtual bool toggleGodMode() = 0;

            virtual bool toggleScripts() = 0;
            virtual bool getScriptsEnabled() const = 0;

            /**
             * @brief startSpellCast attempt to start casting a spell. Might fail immediately if conditions are not met.
             * @param actor
             * @return true if the spell can be casted (i.e. the animation should start)
             */
            virtual bool startSpellCast (const MWWorld::Ptr& actor) = 0;

            virtual void castSpell (const MWWorld::Ptr& actor) = 0;

            virtual void launchMagicBolt (const std::string& model, const std::string& sound, const std::string& spellId,
                                          float speed, bool stack, const ESM::EffectList& effects,
                                           const MWWorld::Ptr& caster, const std::string& sourceName, const osg::Vec3f& fallbackDirection) = 0;
            virtual void launchProjectile (MWWorld::Ptr actor, MWWorld::Ptr projectile,
                                           const osg::Vec3f& worldPos, const osg::Quat& orient, MWWorld::Ptr bow, float speed, float attackStrength) = 0;

            virtual const std::vector<std::string>& getContentFiles() const = 0;

            virtual void breakInvisibility (const MWWorld::Ptr& actor) = 0;

            // Are we in an exterior or pseudo-exterior cell and it's night?
            virtual bool isDark() const = 0;

            virtual bool findInteriorPositionInWorldSpace(MWWorld::CellStore* cell, osg::Vec3f& result) = 0;

            /// Teleports \a ptr to the closest reference of \a id (e.g. DivineMarker, PrisonMarker, TempleMarker)
            /// @note id must be lower case
            virtual void teleportToClosestMarker (const MWWorld::Ptr& ptr,
                                                  const std::string& id) = 0;

            enum DetectionType
            {
                Detect_Enchantment,
                Detect_Key,
                Detect_Creature
            };
            /// List all references (filtered by \a type) detected by \a ptr. The range
            /// is determined by the current magnitude of the "Detect X" magic effect belonging to \a type.
            /// @note This also works for references in containers.
            virtual void listDetectedReferences (const MWWorld::Ptr& ptr, std::vector<MWWorld::Ptr>& out,
                                                  DetectionType type) = 0;

            /// Update the value of some globals according to the world state, which may be used by dialogue entries.
            /// This should be called when initiating a dialogue.
            virtual void updateDialogueGlobals() = 0;

            /// Moves all stolen items from \a ptr to the closest evidence chest.
            virtual void confiscateStolenItems(const MWWorld::Ptr& ptr) = 0;

            virtual void goToJail () = 0;

            /// Spawn a random creature from a levelled list next to the player
            virtual void spawnRandomCreature(const std::string& creatureList) = 0;

            /// Spawn a blood effect for \a ptr at \a worldPosition
            virtual void spawnBloodEffect (const MWWorld::Ptr& ptr, const osg::Vec3f& worldPosition) = 0;

            virtual void spawnEffect (const std::string& model, const std::string& textureOverride, const osg::Vec3f& worldPos) = 0;

            virtual void explodeSpell (const osg::Vec3f& origin, const ESM::EffectList& effects,
                                       const MWWorld::Ptr& caster, ESM::RangeType rangeType, const std::string& id, const std::string& sourceName) = 0;

            virtual void activate (const MWWorld::Ptr& object, const MWWorld::Ptr& actor) = 0;

            /// @see MWWorld::WeatherManager::isInStorm
            virtual bool isInStorm() const = 0;

            /// @see MWWorld::WeatherManager::getStormDirection
            virtual osg::Vec3f getStormDirection() const = 0;

            /// Resets all actors in the current active cells to their original location within that cell.
            virtual void resetActors() = 0;

            virtual bool isWalkingOnWater (const MWWorld::Ptr& actor) = 0;

            /// Return a vector aiming the actor's weapon towards a target.
            /// @note The length of the vector is the distance between actor and target.
            virtual osg::Vec3f aimToTarget(const MWWorld::Ptr& actor, const MWWorld::Ptr& target) = 0;
    };
}

#endif
