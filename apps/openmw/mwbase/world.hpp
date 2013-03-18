#ifndef GAME_MWBASE_WORLD_H
#define GAME_MWBASE_WORLD_H

#include <vector>

#include <components/settings/settings.hpp>

#include "../mwworld/globals.hpp"
#include "../mwworld/ptr.hpp"

namespace Ogre
{
    class Vector2;
    class Vector3;
}

namespace OEngine
{
    namespace Render
    {
        class Fader;
    }

    namespace Physic
    {
        class PhysicEngine;
    }
}

namespace ESM
{
    class ESMReader;
    struct Position;
    struct Cell;
    struct Class;
    struct Potion;
    struct Spell;
    struct NPC;
}

namespace MWRender
{
    class ExternalRendering;
    class Animation;
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

    typedef std::vector<std::pair<MWWorld::Ptr,Ogre::Vector3> > PtrMovementList;
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

            enum RenderMode
            {
                Render_CollisionDebug,
                Render_Wireframe,
                Render_Pathgrid,
                Render_Compositors,
                Render_BoundingBoxes
            };

            struct DoorMarker
            {
                std::string name;
                float x, y; // world position
            };

            World() {}

            virtual ~World() {}

            virtual OEngine::Render::Fader* getFader() = 0;
            ///< \ŧodo remove this function. Rendering details should not be exposed.

            virtual MWWorld::CellStore *getExterior (int x, int y) = 0;

            virtual MWWorld::CellStore *getInterior (const std::string& name) = 0;

            virtual void setWaterHeight(const float height) = 0;

            virtual void toggleWater() = 0;

            virtual void adjustSky() = 0;

            virtual void getTriangleBatchCount(unsigned int &triangles, unsigned int &batches) = 0;

            virtual const MWWorld::Fallback *getFallback () const = 0;

            virtual MWWorld::Player& getPlayer() = 0;

            virtual const MWWorld::ESMStore& getStore() const = 0;

            virtual std::vector<ESM::ESMReader>& getEsmReader() = 0;

            virtual MWWorld::LocalScripts& getLocalScripts() = 0;

            virtual bool hasCellChanged() const = 0;
            ///< Has the player moved to a different cell, since the last frame?

            virtual bool isCellExterior() const = 0;

            virtual bool isCellQuasiExterior() const = 0;

            virtual Ogre::Vector2 getNorthVector (MWWorld::CellStore* cell) = 0;
            ///< get north vector (OGRE coordinates) for given interior cell

            virtual std::vector<DoorMarker> getDoorMarkers (MWWorld::CellStore* cell) = 0;
            ///< get a list of teleport door markers for a given cell, to be displayed on the local map

            virtual void getInteriorMapPosition (Ogre::Vector2 position, float& nX, float& nY, int &x, int& y) = 0;
            ///< see MWRender::LocalMap::getInteriorMapPosition

            virtual bool isPositionExplored (float nX, float nY, int x, int y, bool interior) = 0;
            ///< see MWRender::LocalMap::isPositionExplored

            virtual MWWorld::Globals::Data& getGlobalVariable (const std::string& name) = 0;

            virtual MWWorld::Globals::Data getGlobalVariable (const std::string& name) const = 0;

            virtual char getGlobalVariableType (const std::string& name) const = 0;
            ///< Return ' ', if there is no global variable with this name.

            virtual std::vector<std::string> getGlobals () const = 0;

            virtual std::string getCurrentCellName() const = 0;

            virtual void removeRefScript (MWWorld::RefData *ref) = 0;
            //< Remove the script attached to ref from mLocalScripts

            virtual MWWorld::Ptr getPtr (const std::string& name, bool activeOnly) = 0;
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            virtual MWWorld::Ptr getPtrViaHandle (const std::string& handle) = 0;
            ///< Return a pointer to a liveCellRef with the given Ogre handle.

            virtual MWWorld::Ptr searchPtrViaHandle (const std::string& handle) = 0;
            ///< Return a pointer to a liveCellRef with the given Ogre handle or Ptr() if not found

            /// \todo enable reference in the OGRE scene
            virtual void enable (const MWWorld::Ptr& ptr) = 0;

            /// \todo disable reference in the OGRE scene
            virtual void disable (const MWWorld::Ptr& ptr) = 0;

            virtual void advanceTime (double hours) = 0;
            ///< Advance in-game time.

            virtual void setHour (double hour) = 0;
            ///< Set in-game time hour.

            virtual void setMonth (int month) = 0;
            ///< Set in-game time month.

            virtual void setDay (int day) = 0;
            ///< Set in-game time day.

            virtual int getDay() = 0;
            virtual int getMonth() = 0;

            virtual MWWorld::TimeStamp getTimeStamp() const = 0;
            ///< Return current in-game time stamp.

            virtual bool toggleSky() = 0;
            ///< \return Resulting mode

            virtual void changeWeather(const std::string& region, unsigned int id) = 0;

            virtual int getCurrentWeather() const = 0;

            virtual int getMasserPhase() const = 0;

            virtual int getSecundaPhase() const = 0;

            virtual void setMoonColour (bool red) = 0;

            virtual float getTimeScaleFactor() const = 0;

            virtual void changeToInteriorCell (const std::string& cellName,
                const ESM::Position& position) = 0;
            ///< Move to interior cell.

            virtual void changeToExteriorCell (const ESM::Position& position) = 0;
            ///< Move to exterior cell.

            virtual const ESM::Cell *getExterior (const std::string& cellName) const = 0;
            ///< Return a cell matching the given name or a 0-pointer, if there is no such cell.

            virtual void markCellAsUnchanged() = 0;

            virtual MWWorld::Ptr  getFacedObject() = 0;
            ///< Return pointer to the object the player is looking at, if it is within activation range

            virtual void deleteObject (const MWWorld::Ptr& ptr) = 0;

            virtual void moveObject (const MWWorld::Ptr& ptr, float x, float y, float z) = 0;

            virtual void
            moveObject(const MWWorld::Ptr &ptr, MWWorld::CellStore &newCell, float x, float y, float z) = 0;

            virtual void scaleObject (const MWWorld::Ptr& ptr, float scale) = 0;

            virtual void rotateObject(const MWWorld::Ptr& ptr,float x,float y,float z, bool adjust = false) = 0;

            virtual void safePlaceObject(const MWWorld::Ptr& ptr,MWWorld::CellStore &Cell,ESM::Position pos) = 0;
            ///< place an object in a "safe" location (ie not in the void, etc).

            virtual void indexToPosition (int cellX, int cellY, float &x, float &y, bool centre = false)
                const = 0;
            ///< Convert cell numbers to position.

            virtual void positionToIndex (float x, float y, int &cellX, int &cellY) const = 0;
            ///< Convert position to cell numbers

            virtual void doPhysics (const MWWorld::PtrMovementList &actors, float duration) = 0;
            ///< Run physics simulation and modify \a world accordingly.

            virtual bool toggleCollisionMode() = 0;
            ///< Toggle collision mode for player. If disabled player object should ignore
            /// collisions and gravity.
            /// \return Resulting mode

            virtual bool toggleRenderMode (RenderMode mode) = 0;
            ///< Toggle a render mode.
            ///< \return Resulting mode

            virtual const ESM::Potion *createRecord (const ESM::Potion& record)
                = 0;
            ///< Create a new recrod (of type potion) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Spell *createRecord (const ESM::Spell& record)
                = 0;
            ///< Create a new recrod (of type spell) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Class *createRecord (const ESM::Class& record)
                = 0;
            ///< Create a new recrod (of type class) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Cell *createRecord (const ESM::Cell& record) = 0;
            ///< Create a new recrod (of type cell) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::NPC *createRecord(const ESM::NPC &record) = 0;
            ///< Create a new recrod (of type npc) in the ESM store.
            /// \return pointer to created record

            virtual void update (float duration, bool paused) = 0;

            virtual bool placeObject(const MWWorld::Ptr& object, float cursorX, float cursorY) = 0;
            ///< place an object into the gameworld at the specified cursor position
            /// @param object
            /// @param cursor X (relative 0-1)
            /// @param cursor Y (relative 0-1)
            /// @return true if the object was placed, or false if it was rejected because the position is too far away

            virtual void dropObjectOnGround (const MWWorld::Ptr& actor, const MWWorld::Ptr& object) = 0;

            virtual bool canPlaceObject (float cursorX, float cursorY) = 0;
            ///< @return true if it is possible to place on object at specified cursor location

            virtual void processChangedSettings (const Settings::CategorySettingVector& settings) = 0;

            virtual bool isFlying(const MWWorld::Ptr &ptr) const = 0;
            virtual bool isSwimming(const MWWorld::Ptr &object) const = 0;
            virtual bool isUnderwater(const MWWorld::Ptr::CellStore* cell, const Ogre::Vector3 &pos) const = 0;
            virtual bool isOnGround(const MWWorld::Ptr &ptr) const = 0;

            virtual void togglePOV() = 0;
            virtual void togglePreviewMode(bool enable) = 0;
            virtual bool toggleVanityMode(bool enable, bool force) = 0;
            virtual void allowVanityMode(bool allow) = 0;
            virtual void togglePlayerLooking(bool enable) = 0;
            virtual void changeVanityModeScale(float factor) = 0;

            virtual void renderPlayer() = 0;

            virtual void setupExternalRendering (MWRender::ExternalRendering& rendering) = 0;

            virtual int canRest() = 0;
            ///< check if the player is allowed to rest \n
            /// 0 - yes \n
            /// 1 - only waiting \n
            /// 2 - player is underwater \n
            /// 3 - enemies are nearby (not implemented)

            /// \todo Probably shouldn't be here
            virtual MWRender::Animation* getAnimation(const MWWorld::Ptr &ptr) = 0;

            /// \todo this does not belong here
            virtual void playVideo(const std::string& name, bool allowSkipping) = 0;
            virtual void stopVideo() = 0;
            virtual void frameStarted (float dt) = 0;
    };
}

#endif
