#ifndef GAME_MWWORLD_WORLD_H
#define GAME_MWWORLD_WORLD_H

#include <vector>
#include <map>

#include <boost/filesystem.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwrender/mwscene.hpp"
#include "../mwrender/rendering_manager.hpp"

#include "refdata.hpp"
#include "ptr.hpp"
#include "globals.hpp"
#include "scene.hpp"
#include "physicssystem.hpp"
#include "cells.hpp"
#include "localscripts.hpp"

#include <openengine/bullet/physic.hpp>

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
    class Environment;
    class Player;

    /// \brief The game world and its visual representation

    class World
    {
        public:

            enum RenderMode
            {
                Render_CollisionDebug
            };

        private:

            MWRender::MWScene mScene;
            MWWorld::Scene *mWorldScene;
            MWWorld::Player *mPlayer;
            ESM::ESMReader mEsm;
            ESMS::ESMStore mStore;
            LocalScripts mLocalScripts;
            MWWorld::Globals *mGlobalVariables;
            MWWorld::PhysicsSystem *mPhysics;
            bool mSky;
            Environment& mEnvironment;
            MWRender::RenderingManager *mRenderingManager;
            int mNextDynamicRecord;

            Cells mCells;

            OEngine::Physic::PhysicEngine* mPhysEngine;

            // not implemented
            World (const World&);
            World& operator= (const World&);

            Ptr getPtrViaHandle (const std::string& handle, Ptr::CellStore& cellStore);

            MWRender::CellRender *searchRender (Ptr::CellStore *store);

            int getDaysPerMonth (int month) const;

            void moveObjectImp (Ptr ptr, float x, float y, float z);

        public:

           World (OEngine::Render::OgreRenderer& renderer, OEngine::Physic::PhysicEngine* physEng,
                const Files::Collections& fileCollections,
                const std::string& master, const boost::filesystem::path& resDir, bool newGame,
                Environment& environment, const std::string& encoding);

            ~World();

            Ptr::CellStore *getExterior (int x, int y);

            Ptr::CellStore *getInterior (const std::string& name);

            void adjustSky();

            MWWorld::Player& getPlayer();

            const ESMS::ESMStore& getStore() const;

            ESM::ESMReader& getEsmReader();

            LocalScripts& getLocalScripts();

            bool hasCellChanged() const;
            ///< Has the player moved to a different cell, since the last frame?

            Globals::Data& getGlobalVariable (const std::string& name);

            Globals::Data getGlobalVariable (const std::string& name) const;

            char getGlobalVariableType (const std::string& name) const;
            ///< Return ' ', if there is no global variable with this name.

            Ptr getPtr (const std::string& name, bool activeOnly);
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            Ptr getPtrViaHandle (const std::string& handle);
            ///< Return a pointer to a liveCellRef with the given Ogre handle.

            void enable (Ptr reference);

            void disable (Ptr reference);

            void advanceTime (double hours);

            void setHour (double hour);

            void setMonth (int month);

            void setDay (int day);

            bool toggleSky();
            ///< \return Resulting mode

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
    };
}

#endif
