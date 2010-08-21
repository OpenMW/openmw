#ifndef GAME_MWWORLD_WORLD_H
#define GAME_MWWORLD_WORLD_H

#include <vector>
#include <map>

#include <boost/filesystem.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwrender/playerpos.hpp"
#include "../mwrender/mwscene.hpp"

#include "refdata.hpp"
#include "ptr.hpp"
#include "globals.hpp"

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

    /// \brief The game world and its visual representation

    class World
    {
        public:

            typedef std::list<std::pair<std::string, Ptr> > ScriptList;

        private:

            typedef std::map<Ptr::CellStore *, MWRender::CellRender *> CellRenderCollection;

            MWRender::SkyManager* mSkyManager;
            MWRender::MWScene mScene;
            MWRender::PlayerPos *mPlayerPos;
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

        public:

           World (OEngine::Render::OgreRenderer& renderer, const boost::filesystem::path& master,
                const std::string& dataDir, bool newGame, Environment& environment);

            ~World();

            MWRender::PlayerPos& getPlayerPos();

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

            void changeCell (const std::string& cellName, const ESM::Position& position);
            ///< works only for interior cells currently.

            void changeCell (int X, int Y, const ESM::Position& position);

            void changeToExteriorCell (const ESM::Position& position);

            void markCellAsUnchanged();

            std::string getFacedHandle();
            ///< Return handle of the object the player is looking at

            void deleteObject (Ptr ptr);

            void moveObject (Ptr ptr, float x, float y, float z);
    };
}

#endif
