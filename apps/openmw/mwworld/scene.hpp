#ifndef GAME_MWWORLD_SCENE_H
#define GAME_MWWORLD_SCENE_H

#include "../mwrender/renderingmanager.hpp"

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

namespace Loading
{
    class Listener;
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
    class PhysicsSystem;
    class Player;
    class CellStore;

    class Scene
    {
        public:

            typedef std::set<CellStore *> CellStoreCollection;

        private:

            //OEngine::Render::OgreRenderer& mRenderer;
            CellStore* mCurrentCell; // the cell the player is in
            CellStoreCollection mActiveCells;
            bool mCellChanged;
            PhysicsSystem *mPhysics;
            MWRender::RenderingManager& mRendering;

            bool mNeedMapUpdate;

            void insertCell (CellStore &cell, bool rescale, Loading::Listener* loadingListener);

            // Load and unload cells as necessary to create a cell grid with "X" and "Y" in the center
            void changeCellGrid (int X, int Y);

            void getGridCenter(int& cellX, int& cellY);

        public:

            Scene (MWRender::RenderingManager& rendering, PhysicsSystem *physics);

            ~Scene();

            void unloadCell (CellStoreCollection::iterator iter);

            void loadCell (CellStore *cell, Loading::Listener* loadingListener);

            void playerMoved (const Ogre::Vector3& pos);

            void changePlayerCell (CellStore* newCell, const ESM::Position& position, bool adjustPlayerPos);

            CellStore *getCurrentCell();

            const CellStoreCollection& getActiveCells () const;

            bool hasCellChanged() const;
            ///< Has the set of active cells changed, since the last frame?

            void changeToInteriorCell (const std::string& cellName, const ESM::Position& position);
            ///< Move to interior cell.

            void changeToExteriorCell (const ESM::Position& position, bool adjustPlayerPos);
            ///< Move to exterior cell.

            void changeToVoid();
            ///< Change into a void

            void markCellAsUnchanged();

            void update (float duration, bool paused);

            void addObjectToScene (const Ptr& ptr);
            ///< Add an object that already exists in the world model to the scene.

            void removeObjectFromScene (const Ptr& ptr);
            ///< Remove an object from the scene, but not from the world model.

            void updateObjectLocalRotation (const Ptr& ptr);

            void updateObjectRotation (const Ptr& ptr);

            bool isCellActive(const CellStore &cell);

            Ptr searchPtrViaHandle (const std::string& handle);

            Ptr searchPtrViaActorId (int actorId);
    };
}

#endif
