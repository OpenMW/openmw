#ifndef GAME_MWWORLD_SCENE_H
#define GAME_MWWORLD_SCENE_H

#include <vector>
#include <map>

#include <boost/filesystem.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwrender/mwscene.hpp"
#include "physicssystem.hpp"

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

        private:

            typedef std::map<Ptr::CellStore *, MWRender::CellRender *> CellRenderCollection;

            MWRender::MWScene mScene;
            Ptr::CellStore *mCurrentCell; // the cell, the player is in
            CellRenderCollection mActiveCells;
            bool mCellChanged;
            Environment& mEnvironment;
            World *mWorld;
            PhysicsSystem *mPhysics;


            void playerCellChange (Ptr::CellStore *cell, const ESM::Position& position,
                bool adjustPlayerPos = true);
        public:

           Scene (Environment& environment, World *world, MWRender::MWScene& scene, PhysicsSystem *physics);

            ~Scene();

            void unloadCell (CellRenderCollection::iterator iter);

            void loadCell (Ptr::CellStore *cell, MWRender::CellRender *render);

            void changeCell (int X, int Y, const ESM::Position& position, bool adjustPlayerPos);
            ///< Move from exterior to interior or from interior cell to a different
            /// interior cell.
            
            Ptr::CellStore* getCurrentCell ();
            
            CellRenderCollection getActiveCells ();

            bool hasCellChanged() const;
            ///< Has the player moved to a different cell, since the last frame?

            void changeToInteriorCell (const std::string& cellName, const ESM::Position& position);
            ///< Move to interior cell.

            void changeToExteriorCell (const ESM::Position& position); 
            ///< Move to exterior cell.

            void markCellAsUnchanged(); 

            std::string getFacedHandle();
    };
}

#endif
