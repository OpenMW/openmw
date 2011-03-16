#ifndef GAME_MWWORLD_WORLD_H
#define GAME_MWWORLD_WORLD_H

#include <vector>
#include <map>

#include <boost/filesystem.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwrender/mwscene.hpp"

#include "refdata.hpp"
#include "ptr.hpp"
#include "globals.hpp"

#include <openengine/bullet/physic.hpp>

namespace Ogre
{
    class Vector3;
}

namespace ESM
{
    struct Position;
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
            typedef std::list<std::pair<std::string, Ptr> > ScriptList;

            enum RenderMode
            {
                Render_CollisionDebug
            };

        private:

            typedef std::map<Ptr::CellStore *, MWRender::CellRender *> CellRenderCollection;

            MWRender::SkyManager* mSkyManager;
            MWRender::MWScene mScene;
            MWWorld::Player *mPlayer;
            Ptr::CellStore *mCurrentCell; // the cell, the player is in
            CellRenderCollection mActiveCells;
            CellRenderCollection mBufferedCells; // loaded, but not active (buffering not implementd yet)
            ESM::ESMReader mEsm;
            ESMS::ESMStore mStore;
            std::map<std::string, Ptr::CellStore> mInteriors;
            std::map<std::pair<int, int>, Ptr::CellStore> mExteriors;
            ScriptList mLocalScripts;
            MWWorld::Globals *mGlobalVariables;
            bool mSky;
            bool mCellChanged;
            Environment& mEnvironment;

			OEngine::Physic::PhysicEngine* mPhysEngine;

            // not implemented
            World (const World&);
            World& operator= (const World&);

            void insertInteriorScripts (ESMS::CellStore<RefData>& cell);

            Ptr getPtr (const std::string& name, Ptr::CellStore& cellStore);

            Ptr getPtrViaHandle (const std::string& handle, Ptr::CellStore& cellStore);

            MWRender::CellRender *searchRender (Ptr::CellStore *store);

            int getDaysPerMonth (int month) const;

            void removeScripts (Ptr::CellStore *cell);

            void unloadCell (CellRenderCollection::iterator iter);

            void loadCell (Ptr::CellStore *cell, MWRender::CellRender *render);

            void playerCellChange (Ptr::CellStore *cell, const ESM::Position& position,
                bool adjustPlayerPos = true);

            void adjustSky();

            void changeCell (int X, int Y, const ESM::Position& position, bool adjustPlayerPos);
            ///< Move from exterior to interior or from interior cell to a different
            /// interior cell.
        public:

           World (OEngine::Render::OgreRenderer& renderer, OEngine::Physic::PhysicEngine* physEng, const boost::filesystem::path& dataDir,
                const std::string& master, const boost::filesystem::path& resDir, bool newGame,
                Environment& environment);

            ~World();

            MWWorld::Player& getPlayer();

            ESMS::ESMStore& getStore();

            const ScriptList& getLocalScripts() const;
            ///< Names and local variable state of all local scripts in active cells.

            bool hasCellChanged() const;
            ///< Has the player moved to a different cell, since the last frame?

            Globals::Data& getGlobalVariable (const std::string& name);

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

            void toggleSky();

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

            void toggleCollisionMode();
            ///< Toggle collision mode for player. If disabled player object should ignore
            /// collisions and gravity.

            void toggleRenderMode (RenderMode mode);
            ///< Toggle a render mode.
    };
}

#endif
