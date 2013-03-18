#ifndef GAME_MWWORLD_WORLDIMP_H
#define GAME_MWWORLD_WORLDIMP_H

#include "../mwrender/debugging.hpp"

#include "ptr.hpp"
#include "scene.hpp"
#include "esmstore.hpp"
#include "physicssystem.hpp"
#include "cells.hpp"
#include "localscripts.hpp"
#include "timestamp.hpp"
#include "fallback.hpp"

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
    class Animation;
}

namespace MWWorld
{
    class WeatherManager;
    class Player;

    /// \brief The game world and its visual representation

    class World : public MWBase::World
    {
            MWWorld::Fallback mFallback;
            MWRender::RenderingManager* mRendering;

            MWWorld::WeatherManager* mWeatherManager;

            MWWorld::Scene *mWorldScene;
            MWWorld::Player *mPlayer;
            std::vector<ESM::ESMReader> mEsm;
            MWWorld::ESMStore mStore;
            LocalScripts mLocalScripts;
            MWWorld::Globals *mGlobalVariables;
            MWWorld::PhysicsSystem *mPhysics;
            bool mSky;

            Cells mCells;

            OEngine::Physic::PhysicEngine* mPhysEngine;

            // not implemented
            World (const World&);
            World& operator= (const World&);

            Ptr getPtrViaHandle (const std::string& handle, Ptr::CellStore& cellStore);

            int mActivationDistanceOverride;
            std::string mFacedHandle;
            float mFacedDistance;
            Ptr mFaced1;
            Ptr mFaced2;
            std::string mFaced1Name;
            std::string mFaced2Name;
            float mFaced1Distance;
            float mFaced2Distance;
            int mNumFacing;

            unsigned long lastTick;
            Ogre::Timer mTimer;

            int getDaysPerMonth (int month) const;

            bool moveObjectImp (const Ptr& ptr, float x, float y, float z);
            ///< @return true if the active cell (cell player is in) changed


            Ptr copyObjectToCell(const Ptr &ptr, CellStore &cell, const ESM::Position &pos);

            void updateWindowManager ();
            void performUpdateSceneQueries ();
            void updateFacedHandle ();

            float getMaxActivationDistance ();
            float getNpcActivationDistance ();
            float getObjectActivationDistance ();

            void removeContainerScripts(const Ptr& reference);
            void addContainerScripts(const Ptr& reference, Ptr::CellStore* cell);
            void PCDropped (const Ptr& item);

        public:

            World (OEngine::Render::OgreRenderer& renderer,
                const Files::Collections& fileCollections,
                const std::vector<std::string>& master, const std::vector<std::string>& plugins,
        	const boost::filesystem::path& resDir, const boost::filesystem::path& cacheDir, bool newGame,
                ToUTF8::Utf8Encoder* encoder, const std::map<std::string,std::string>& fallbackMap, int mActivationDistanceOverride);

            virtual ~World();

            virtual OEngine::Render::Fader* getFader();
            ///< \ŧodo remove this function. Rendering details should not be exposed.

            virtual CellStore *getExterior (int x, int y);

            virtual CellStore *getInterior (const std::string& name);

            virtual void setWaterHeight(const float height);

            virtual void toggleWater();

            virtual void adjustSky();

            virtual void getTriangleBatchCount(unsigned int &triangles, unsigned int &batches);

            virtual const Fallback *getFallback() const;

            virtual Player& getPlayer();

            virtual const MWWorld::ESMStore& getStore() const;

            virtual std::vector<ESM::ESMReader>& getEsmReader();

            virtual LocalScripts& getLocalScripts();

            virtual bool hasCellChanged() const;
            ///< Has the player moved to a different cell, since the last frame?

            virtual bool isCellExterior() const;

            virtual bool isCellQuasiExterior() const;

            virtual Ogre::Vector2 getNorthVector (CellStore* cell);
            ///< get north vector (OGRE coordinates) for given interior cell

            virtual std::vector<DoorMarker> getDoorMarkers (MWWorld::CellStore* cell);
            ///< get a list of teleport door markers for a given cell, to be displayed on the local map

            virtual void getInteriorMapPosition (Ogre::Vector2 position, float& nX, float& nY, int &x, int& y);
            ///< see MWRender::LocalMap::getInteriorMapPosition

            virtual bool isPositionExplored (float nX, float nY, int x, int y, bool interior);
            ///< see MWRender::LocalMap::isPositionExplored

            virtual Globals::Data& getGlobalVariable (const std::string& name);

            virtual Globals::Data getGlobalVariable (const std::string& name) const;

            virtual char getGlobalVariableType (const std::string& name) const;
            ///< Return ' ', if there is no global variable with this name.

            virtual std::vector<std::string> getGlobals () const;

            virtual std::string getCurrentCellName () const;

            virtual void removeRefScript (MWWorld::RefData *ref);
            //< Remove the script attached to ref from mLocalScripts

            virtual Ptr getPtr (const std::string& name, bool activeOnly);
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            virtual Ptr getPtrViaHandle (const std::string& handle);
            ///< Return a pointer to a liveCellRef with the given Ogre handle.

            virtual Ptr searchPtrViaHandle (const std::string& handle);
            ///< Return a pointer to a liveCellRef with the given Ogre handle or Ptr() if not found

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

            virtual int getDay();
            virtual int getMonth();

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

            virtual MWWorld::Ptr getFacedObject();
            ///< Return pointer to the object the player is looking at, if it is within activation range

            virtual void deleteObject (const Ptr& ptr);

            virtual void moveObject (const Ptr& ptr, float x, float y, float z);
            virtual void moveObject (const Ptr& ptr, CellStore &newCell, float x, float y, float z);

            virtual void scaleObject (const Ptr& ptr, float scale);

            /// Rotates object, uses degrees
            /// \param adjust indicates rotation should be set or adjusted
            virtual void rotateObject (const Ptr& ptr,float x,float y,float z, bool adjust = false);

            virtual void safePlaceObject(const MWWorld::Ptr& ptr,MWWorld::CellStore &Cell,ESM::Position pos);
            ///< place an object in a "safe" location (ie not in the void, etc). Makes a copy of the Ptr.

            virtual void indexToPosition (int cellX, int cellY, float &x, float &y, bool centre = false)
                const;
            ///< Convert cell numbers to position.

            virtual void positionToIndex (float x, float y, int &cellX, int &cellY) const;
            ///< Convert position to cell numbers

            virtual void doPhysics(const PtrMovementList &actors, float duration);
            ///< Run physics simulation and modify \a world accordingly.

            virtual bool toggleCollisionMode();
            ///< Toggle collision mode for player. If disabled player object should ignore
            /// collisions and gravity.
            ///< \return Resulting mode

            virtual bool toggleRenderMode (RenderMode mode);
            ///< Toggle a render mode.
            ///< \return Resulting mode

            virtual const ESM::Potion *createRecord (const ESM::Potion& record);
            ///< Create a new recrod (of type potion) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Spell *createRecord (const ESM::Spell& record);
            ///< Create a new recrod (of type spell) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Class *createRecord (const ESM::Class& record);
            ///< Create a new recrod (of type class) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::Cell *createRecord (const ESM::Cell& record);
            ///< Create a new recrod (of type cell) in the ESM store.
            /// \return pointer to created record

            virtual const ESM::NPC *createRecord(const ESM::NPC &record);
            ///< Create a new recrod (of type npc) in the ESM store.
            /// \return pointer to created record


            virtual void update (float duration, bool paused);

            virtual bool placeObject (const Ptr& object, float cursorX, float cursorY);
            ///< place an object into the gameworld at the specified cursor position
            /// @param object
            /// @param cursor X (relative 0-1)
            /// @param cursor Y (relative 0-1)
            /// @return true if the object was placed, or false if it was rejected because the position is too far away

            virtual void dropObjectOnGround (const Ptr& actor, const Ptr& object);

            virtual bool canPlaceObject(float cursorX, float cursorY);
            ///< @return true if it is possible to place on object at specified cursor location

            virtual void processChangedSettings(const Settings::CategorySettingVector& settings);

            virtual bool isFlying(const MWWorld::Ptr &ptr) const;
            virtual bool isSwimming(const MWWorld::Ptr &object) const;
            virtual bool isUnderwater(const MWWorld::Ptr::CellStore* cell, const Ogre::Vector3 &pos) const;
            virtual bool isOnGround(const MWWorld::Ptr &ptr) const;

            virtual void togglePOV() {
                mRendering->togglePOV();
            }

            virtual void togglePreviewMode(bool enable) {
                mRendering->togglePreviewMode(enable);
            }

            virtual bool toggleVanityMode(bool enable, bool force) {
                return mRendering->toggleVanityMode(enable, force);
            }

            virtual void allowVanityMode(bool allow) {
                mRendering->allowVanityMode(allow);
            }

            virtual void togglePlayerLooking(bool enable) {
                mRendering->togglePlayerLooking(enable);
            }

            virtual void changeVanityModeScale(float factor) {
                mRendering->changeVanityModeScale(factor);
            }

            virtual void renderPlayer();

            virtual void setupExternalRendering (MWRender::ExternalRendering& rendering);

            virtual int canRest();
            ///< check if the player is allowed to rest \n
            /// 0 - yes \n
            /// 1 - only waiting \n
            /// 2 - player is underwater \n
            /// 3 - enemies are nearby (not implemented)

            /// \todo Probably shouldn't be here
            virtual MWRender::Animation* getAnimation(const MWWorld::Ptr &ptr);

            /// \todo this does not belong here
            virtual void playVideo(const std::string& name, bool allowSkipping);
            virtual void stopVideo();
            virtual void frameStarted (float dt);
    };
}

#endif
