#ifndef GAME_MWBASE_WORLD_H
#define GAME_MWBASE_WORLD_H

#include <vector>

#include <components/settings/settings.hpp>

#include "../mwworld/globals.hpp"

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
}

namespace ESM
{
    class ESMReader;
    struct Position;
    struct Cell;
    struct Class;
    struct Potion;
}

namespace ESMS
{
    struct ESMStore;
}

namespace MWWorld
{
    class CellStore;
    class Player;
    class LocalScripts;
    class Ptr;
    class TimeStamp;
}

namespace MWBase
{
    class World
    {
            World (const World&);
            ///< not implemented

            World& operator= (const World&);
            ///< not implemented

        protected:

            virtual void
            placeObject(
                const MWWorld::Ptr &ptr,
                MWWorld::CellStore &cell,
                const ESM::Position &pos) = 0;

        public:

            enum RenderMode
            {
                Render_CollisionDebug,
                Render_Wireframe,
                Render_Pathgrid,
                Render_Compositors
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

            virtual void setFallbackValues (const std::map<std::string, std::string>& fallbackMap) = 0;

            virtual std::string getFallback (const std::string& key) const = 0;

            virtual std::string getFallback (const std::string& key, const std::string& def) const = 0;

            virtual MWWorld::Player& getPlayer() = 0;

            virtual const ESMS::ESMStore& getStore() const = 0;

            virtual ESM::ESMReader& getEsmReader() = 0;

            virtual MWWorld::LocalScripts& getLocalScripts() = 0;

            virtual bool hasCellChanged() const = 0;
            ///< Has the player moved to a different cell, since the last frame?

            virtual bool isCellExterior() const = 0;

            virtual bool isCellQuasiExterior() const = 0;

            virtual Ogre::Vector2 getNorthVector (MWWorld::CellStore* cell) = 0;
            ///< get north vector (OGRE coordinates) for given interior cell

            virtual MWWorld::Globals::Data& getGlobalVariable (const std::string& name) = 0;

            virtual MWWorld::Globals::Data getGlobalVariable (const std::string& name) const = 0;

            virtual char getGlobalVariableType (const std::string& name) const = 0;
            ///< Return ' ', if there is no global variable with this name.

            virtual MWWorld::Ptr getPtr (const std::string& name, bool activeOnly) = 0;
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            virtual MWWorld::Ptr getPtrViaHandle (const std::string& handle) = 0;
            ///< Return a pointer to a liveCellRef with the given Ogre handle.

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

            virtual std::string getFacedHandle() = 0;
            ///< Return handle of the object the player is looking at

            virtual void deleteObject (const MWWorld::Ptr& ptr) = 0;

            virtual void moveObject (const MWWorld::Ptr& ptr, float x, float y, float z) = 0;

            virtual void scaleObject (const MWWorld::Ptr& ptr, float scale) = 0;

            virtual void rotateObject(const MWWorld::Ptr& ptr,float x,float y,float z) = 0;

            virtual void indexToPosition (int cellX, int cellY, float &x, float &y, bool centre = false)
                const = 0;
            ///< Convert cell numbers to position.

            virtual void positionToIndex (float x, float y, int &cellX, int &cellY) const = 0;
            ///< Convert position to cell numbers

            virtual void doPhysics (const std::vector<std::pair<std::string, Ogre::Vector3> >& actors,
                float duration) = 0;
            ///< Run physics simulation and modify \a world accordingly.

            virtual bool toggleCollisionMode() = 0;
            ///< Toggle collision mode for player. If disabled player object should ignore
            /// collisions and gravity.
            ///< \return Resulting mode

            virtual bool toggleRenderMode (RenderMode mode) = 0;
            ///< Toggle a render mode.
            ///< \return Resulting mode

            virtual std::pair<std::string, const ESM::Potion *> createRecord (const ESM::Potion& record)
                = 0;
            ///< Create a new recrod (of type potion) in the ESM store.
            /// \return ID, pointer to created record

            virtual std::pair<std::string, const ESM::Class *> createRecord (const ESM::Class& record)
                = 0;
            ///< Create a new recrod (of type class) in the ESM store.
            /// \return ID, pointer to created record

            virtual const ESM::Cell *createRecord (const ESM::Cell& record) = 0;
            ///< Create a new recrod (of type cell) in the ESM store.
            /// \return ID, pointer to created record

            virtual void playAnimationGroup (const MWWorld::Ptr& ptr, const std::string& groupName,
                int mode, int number = 1) = 0;
            ///< Run animation for a MW-reference. Calls to this function for references that are
            /// currently not in the rendered scene should be ignored.
            ///
            /// \param mode: 0 normal, 1 immediate start, 2 immediate loop
            /// \param number How offen the animation should be run

            virtual void skipAnimation (const MWWorld::Ptr& ptr) = 0;
            ///< Skip the animation for the given MW-reference for one frame. Calls to this function for
            /// references that are currently not in the rendered scene should be ignored.

            virtual void update (float duration) = 0;

            virtual bool placeObject(const MWWorld::Ptr& object, float cursorX, float cursorY) = 0;
            ///< place an object into the gameworld at the specified cursor position
            /// @param object
            /// @param cursor X (relative 0-1)
            /// @param cursor Y (relative 0-1)
            /// @return true if the object was placed, or false if it was rejected because the position is too far away

            virtual void dropObjectOnGround (const MWWorld::Ptr& object) = 0;

            virtual bool canPlaceObject (float cursorX, float cursorY) = 0;
            ///< @return true if it is possible to place on object at specified cursor location

            virtual void processChangedSettings (const Settings::CategorySettingVector& settings) = 0;
    };
}

#endif
