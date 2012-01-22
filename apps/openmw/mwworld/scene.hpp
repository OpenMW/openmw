#ifndef GAME_MWWORLD_SCENE_H
#define GAME_MWWORLD_SCENE_H

#include <vector>
#include <map>

#include <boost/filesystem.hpp>

#include <openengine/bullet/physic.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "physicssystem.hpp"
#include "refdata.hpp"
#include "ptr.hpp"
#include "globals.hpp"

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

    class Scene
    {

        public:

            typedef std::set<Ptr::CellStore *> CellStoreCollection;

        private:

            //OEngine::Render::OgreRenderer& mRenderer;
            Ptr::CellStore* mCurrentCell; // the cell, the player is in
            CellStoreCollection mActiveCells;
            bool mCellChanged;
            Environment& mEnvironment;
            World *mWorld;
            PhysicsSystem *mPhysics;
            MWRender::RenderingManager& mRendering;

            void playerCellChange (Ptr::CellStore *cell, const ESM::Position& position,
                bool adjustPlayerPos = true);


        public:

            Scene (Environment& environment, World *world, MWRender::RenderingManager& rendering, PhysicsSystem *physics);

            ~Scene();

            void unloadCell (CellStoreCollection::iterator iter);

            void loadCell (Ptr::CellStore *cell);

            void changeCell (int X, int Y, const ESM::Position& position, bool adjustPlayerPos);
            ///< Move from exterior to interior or from interior cell to a different
            /// interior cell.

            Ptr::CellStore* getCurrentCell ();

            const CellStoreCollection& getActiveCells () const;

            bool hasCellChanged() const;
            ///< Has the player moved to a different cell, since the last frame?

            void changeToInteriorCell (const std::string& cellName, const ESM::Position& position);
            ///< Move to interior cell.

            void changeToExteriorCell (const ESM::Position& position);
            ///< Move to exterior cell.

            void markCellAsUnchanged();

            void insertCell(ESMS::CellStore<MWWorld::RefData> &cell, MWWorld::Environment& environment);
    };
}

#endif
