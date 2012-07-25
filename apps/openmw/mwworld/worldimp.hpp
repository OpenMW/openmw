#ifndef GAME_MWWORLD_WORLDIMP_H
#define GAME_MWWORLD_WORLDIMP_H

#include <components/esm_store/store.hpp>

#include "../mwrender/debugging.hpp"

#include "ptr.hpp"
#include "scene.hpp"
#include "physicssystem.hpp"
#include "cells.hpp"
#include "localscripts.hpp"
#include "timestamp.hpp"

#include "../mwbase/world.hpp"

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

    class World : public MWBase::World
    {
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

            bool moveObjectImp (const Ptr& ptr, float x, float y, float z);
            ///< @return true if the active cell (cell player is in) changed

            virtual void
            placeObject(const Ptr &ptr, CellStore &cell, const ESM::Position &pos);

        public:

            World (OEngine::Render::OgreRenderer& renderer,
                const Files::Collections& fileCollections,
                const std::string& master, const boost::filesystem::path& resDir, bool newGame,
                const std::string& encoding, std::map<std::string,std::string> fallbackMap);

            virtual ~World();

            virtual OEngine::Render::Fader* getFader();
            ///< \ŧodo remove this function. Rendering details should not be exposed.

            virtual CellStore *getExterior (int x, int y);

            virtual CellStore *getInterior (const std::string& name);

            virtual void setWaterHeight(const float height);

            virtual void toggleWater();

            virtual void adjustSky();

            virtual void getTriangleBatchCount(unsigned int &triangles, unsigned int &batches);

            virtual void setFallbackValues (const std::map<std::string,std::string>& fallbackMap);

            virtual std::string getFallback (const std::string& key) const;

            virtual std::string getFallback (const std::string& key, const std::string& def) const;

            virtual Player& getPlayer();

            virtual const ESMS::ESMStore& getStore() const;

            virtual ESM::ESMReader& getEsmReader();

            virtual LocalScripts& getLocalScripts();

            virtual bool hasCellChanged() const;
            ///< Has the player moved to a different cell, since the last frame?

            virtual bool isCellExterior() const;

            virtual bool isCellQuasiExterior() const;

            virtual Ogre::Vector2 getNorthVector (CellStore* cell);
            ///< get north vector (OGRE coordinates) for given interior cell

            virtual Globals::Data& getGlobalVariable (const std::string& name);

            virtual Globals::Data getGlobalVariable (const std::string& name) const;

            virtual char getGlobalVariableType (const std::string& name) const;
            ///< Return ' ', if there is no global variable with this name.

            virtual Ptr getPtr (const std::string& name, bool activeOnly);
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            virtual Ptr getPtrViaHandle (const std::string& handle);
            ///< Return a pointer to a liveCellRef with the given Ogre handle.

            virtual void enable (const Ptr& ptr);

            virtual void disable (const Ptr& ptr);

            virtual void advanceTime (double hours);
            ///< Advance in-game time.

            virtual void setHour (double hour);
            ///< Set in-game time hour.

            virtual void setMonth (int month);
            ///< Set in-game time month.

            virtual void setDay (int day);
            ///< Set in-game time day.

            virtual TimeStamp getTimeStamp() const;
            ///< Return current in-game time stamp.

            virtual bool toggleSky();
            ///< \return Resulting mode

            virtual void changeWeather (const std::string& region, unsigned int id);

            virtual int getCurrentWeather() const;

            virtual int getMasserPhase() const;

            virtual int getSecundaPhase() const;

            virtual void setMoonColour (bool red);

            virtual float getTimeScaleFactor() const;

            virtual void changeToInteriorCell (const std::string& cellName,
                const ESM::Position& position);
            ///< Move to interior cell.

            virtual void changeToExteriorCell (const ESM::Position& position);
            ///< Move to exterior cell.

            virtual const ESM::Cell *getExterior (const std::string& cellName) const;
            ///< Return a cell matching the given name or a 0-pointer, if there is no such cell.

            virtual void markCellAsUnchanged();

            virtual std::string getFacedHandle();
            ///< Return handle of the object the player is looking at

            virtual void deleteObject (const Ptr& ptr);

            virtual void moveObject (const Ptr& ptr, float x, float y, float z);

            virtual void scaleObject (const Ptr& ptr, float scale);

            virtual void rotateObject (const Ptr& ptr,float x,float y,float z);

            virtual void indexToPosition (int cellX, int cellY, float &x, float &y, bool centre = false)
                const;
            ///< Convert cell numbers to position.

            virtual void positionToIndex (float x, float y, int &cellX, int &cellY) const;
            ///< Convert position to cell numbers

            virtual void doPhysics (const std::vector<std::pair<std::string, Ogre::Vector3> >& actors,
                float duration);
            ///< Run physics simulation and modify \a world accordingly.

            virtual bool toggleCollisionMode();
            ///< Toggle collision mode for player. If disabled player object should ignore
            /// collisions and gravity.
            ///< \return Resulting mode

            virtual bool toggleRenderMode (RenderMode mode);
            ///< Toggle a render mode.
            ///< \return Resulting mode

            virtual std::pair<std::string, const ESM::Potion *> createRecord (const ESM::Potion& record);
            ///< Create a new recrod (of type potion) in the ESM store.
            /// \return ID, pointer to created record

            virtual std::pair<std::string, const ESM::Class *> createRecord (const ESM::Class& record);
            ///< Create a new recrod (of type class) in the ESM store.
            /// \return ID, pointer to created record

            virtual const ESM::Cell *createRecord (const ESM::Cell& record);
            ///< Create a new recrod (of type cell) in the ESM store.
            /// \return ID, pointer to created record

            virtual void playAnimationGroup (const MWWorld::Ptr& ptr, const std::string& groupName,
                int mode, int number = 1);
            ///< Run animation for a MW-reference. Calls to this function for references that are
            /// currently not in the rendered scene should be ignored.
            ///
            /// \param mode: 0 normal, 1 immediate start, 2 immediate loop
            /// \param number How offen the animation should be run

            virtual void skipAnimation (const MWWorld::Ptr& ptr);
            ///< Skip the animation for the given MW-reference for one frame. Calls to this function for
            /// references that are currently not in the rendered scene should be ignored.

            virtual void update (float duration);

            virtual bool placeObject (const Ptr& object, float cursorX, float cursorY);
            ///< place an object into the gameworld at the specified cursor position
            /// @param object
            /// @param cursor X (relative 0-1)
            /// @param cursor Y (relative 0-1)
            /// @return true if the object was placed, or false if it was rejected because the position is too far away

            virtual void dropObjectOnGround (const Ptr& object);

            virtual bool canPlaceObject(float cursorX, float cursorY);
            ///< @return true if it is possible to place on object at specified cursor location

            virtual void processChangedSettings(const Settings::CategorySettingVector& settings);
    };
}

#endif
