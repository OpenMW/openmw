#ifndef GAME_MWWORLD_WORLD_H
#define GAME_MWWORLD_WORLD_H

#include <vector>
#include <map>

#include <boost/filesystem.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwrender/debugging.hpp"
#include "../mwrender/renderingmanager.hpp"

#include "refdata.hpp"
#include "ptr.hpp"
#include "globals.hpp"
#include "scene.hpp"
#include "physicssystem.hpp"
#include "cells.hpp"
#include "localscripts.hpp"
#include "timestamp.hpp"

#include <openengine/bullet/physic.hpp>
#include <openengine/ogre/fader.hpp>

#include <OgreTimer.h>

namespace Ogre
{
    class Vector3;
}

namespace ESM
{
    struct Position;
}

namespace Files
{
    class Collections;
}

namespace Render
{
    class OgreRenderer;
}

namespace MWRender
{
    class SkyManager;
    class CellRender;
}

namespace MWWorld
{
    class WeatherManager;
    class Player;

    /// \brief The game world and its visual representation

    class World
    {
        public:

            enum RenderMode
            {
                Render_CollisionDebug,
                Render_Wireframe,
                Render_Pathgrid
            };

        private:

            MWRender::RenderingManager* mRendering;

            MWWorld::WeatherManager* mWeatherManager;

            MWWorld::Scene *mWorldScene;
            MWWorld::Player *mPlayer;
            ESM::ESMReader mEsm;
            ESMS::ESMStore mStore;
            LocalScripts mLocalScripts;
            MWWorld::Globals *mGlobalVariables;
            MWWorld::PhysicsSystem *mPhysics;
            bool mSky;
            int mNextDynamicRecord;

            Cells mCells;

            OEngine::Physic::PhysicEngine* mPhysEngine;

            // not implemented
            World (const World&);
            World& operator= (const World&);

            Ptr getPtrViaHandle (const std::string& handle, Ptr::CellStore& cellStore);

            std::string mFacedHandle;
            Ptr mFaced1;
            Ptr mFaced2;
            std::string mFaced1Name;
            std::string mFaced2Name;
            int mNumFacing;
            std::map<std::string,std::string> mFallback;

            unsigned long lastTick;
            Ogre::Timer mTimer;

            int getDaysPerMonth (int month) const;

            bool moveObjectImp (Ptr ptr, float x, float y, float z);
            ///< @return true if the active cell (cell player is in) changed

        public:

           World (OEngine::Render::OgreRenderer& renderer,
                const Files::Collections& fileCollections,
                const std::string& master, const boost::filesystem::path& resDir, bool newGame,
                const std::string& encoding, std::map<std::string,std::string> fallbackMap);

            ~World();

            OEngine::Render::Fader* getFader();

            Ptr::CellStore *getExterior (int x, int y);

            Ptr::CellStore *getInterior (const std::string& name);

            void setWaterHeight(const float height);
            void toggleWater();

            void adjustSky();

            void setFallbackValues(std::map<std::string,std::string> fallbackMap);

            std::string getFallback(std::string key);

            std::string getFallback(std::string key, std::string def);

            MWWorld::Player& getPlayer();

            const ESMS::ESMStore& getStore() const;

            ESM::ESMReader& getEsmReader();

            LocalScripts& getLocalScripts();

            bool hasCellChanged() const;
            ///< Has the player moved to a different cell, since the last frame?

            bool isCellExterior() const;
            bool isCellQuasiExterior() const;

            Ogre::Vector2 getNorthVector(Ptr::CellStore* cell);
            ///< get north vector (OGRE coordinates) for given interior cell

            Globals::Data& getGlobalVariable (const std::string& name);

            Globals::Data getGlobalVariable (const std::string& name) const;

            char getGlobalVariableType (const std::string& name) const;
            ///< Return ' ', if there is no global variable with this name.

            Ptr getPtr (const std::string& name, bool activeOnly);
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            Ptr getPtrViaHandle (const std::string& handle);
            ///< Return a pointer to a liveCellRef with the given Ogre handle.

            /// \todo enable reference in the OGRE scene
            void enable (Ptr reference);

            /// \todo 5disable reference in the OGRE scene
            void disable (Ptr reference);

            void advanceTime (double hours);
            ///< Advance in-game time.

            void setHour (double hour);
            ///< Set in-game time hour.

            void setMonth (int month);
            ///< Set in-game time month.

            void setDay (int day);
            ///< Set in-game time day.

            TimeStamp getTimeStamp() const;
            ///< Return current in-game time stamp.

            bool toggleSky();
            ///< \return Resulting mode

            void changeWeather(const std::string& region, const unsigned int id);

            int getCurrentWeather() const;

            int getMasserPhase() const;

            int getSecundaPhase() const;

            void setMoonColour (bool red);

            float getTimeScaleFactor() const;

            void changeToInteriorCell (const std::string& cellName, const ESM::Position& position);
            ///< Move to interior cell.

            void changeToExteriorCell (const ESM::Position& position);
            ///< Move to exterior cell.

            const ESM::Cell *getExterior (const std::string& cellName) const;
            ///< Return a cell matching the given name or a 0-pointer, if there is no such cell.

            void markCellAsUnchanged();

            std::string getFacedHandle();
            ///< Return handle of the object the player is looking at

            void deleteObject (Ptr ptr);

            void moveObject (Ptr ptr, float x, float y, float z);

            void scaleObject (Ptr ptr, float scale);

            void rotateObject (Ptr ptr,float x,float y,float z);

            void indexToPosition (int cellX, int cellY, float &x, float &y, bool centre = false) const;
            ///< Convert cell numbers to position.

            void positionToIndex (float x, float y, int &cellX, int &cellY) const;
            ///< Convert position to cell numbers

            void doPhysics (const std::vector<std::pair<std::string, Ogre::Vector3> >& actors,
                float duration);
            ///< Run physics simulation and modify \a world accordingly.

            bool toggleCollisionMode();
            ///< Toggle collision mode for player. If disabled player object should ignore
            /// collisions and gravity.
            ///< \return Resulting mode

            bool toggleRenderMode (RenderMode mode);
            ///< Toggle a render mode.
            ///< \return Resulting mode

            std::pair<std::string, const ESM::Potion *> createRecord (const ESM::Potion& record);
            ///< Create a new recrod (of type potion) in the ESM store.
            /// \return ID, pointer to created record

            std::pair<std::string, const ESM::Class *> createRecord (const ESM::Class& record);
            ///< Create a new recrod (of type class) in the ESM store.
            /// \return ID, pointer to created record

            const ESM::Cell *createRecord (const ESM::Cell& record);
            ///< Create a new recrod (of type cell) in the ESM store.
            /// \return ID, pointer to created record

            void playAnimationGroup (const MWWorld::Ptr& ptr, const std::string& groupName, int mode,
                int number = 1);
            ///< Run animation for a MW-reference. Calls to this function for references that are
            /// currently not in the rendered scene should be ignored.
            ///
            /// \param mode: 0 normal, 1 immediate start, 2 immediate loop
            /// \param number How offen the animation should be run

            void skipAnimation (const MWWorld::Ptr& ptr);
            ///< Skip the animation for the given MW-reference for one frame. Calls to this function for
            /// references that are currently not in the rendered scene should be ignored.

            void update (float duration);

            bool placeObject(MWWorld::Ptr object, float cursorX, float cursorY);
            ///< place an object into the gameworld at the specified cursor position
            /// @param object
            /// @param cursor X (relative 0-1)
            /// @param cursor Y (relative 0-1)
            /// @return true if the object was placed, or false if it was rejected because the position is too far away

            void dropObjectOnGround(MWWorld::Ptr object);

            bool canPlaceObject(float cursorX, float cursorY);
            ///< @return true if it is possible to place on object at specified cursor location
    };
}

#endif
