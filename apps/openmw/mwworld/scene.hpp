#ifndef GAME_MWWORLD_SCENE_H
#define GAME_MWWORLD_SCENE_H

#include <components/esm_store/cell_store.hpp>
#include "ptr.hpp"
#include "environment.hpp"
#include "../mwrender/mwscene.hpp"

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

    class Scene
    {
        public:
            Scene(Environment& environment, World *world, MWRender::MWScene scene);
            
        private:
        
            typedef std::map<Ptr::CellStore *, MWRender::CellRender *> CellRenderCollection;
            
            CellRenderCollection mActiveCells;
            Ptr::CellStore *mCurrentCell; // the cell, the player is in
            std::map<std::string, Ptr::CellStore> mInteriors;
            std::map<std::pair<int, int>, Ptr::CellStore> mExteriors;
            Environment& mEnvironment;
            World *mWorld;
            MWRender::MWScene mScene;
            bool mCellChanged;
            
            Ptr getPtr (const std::string& name, Ptr::CellStore& cell);
            Ptr getPtrViaHandle (const std::string& handle, Ptr::CellStore& cell);
            
        public:
            
            MWRender::CellRender *searchRender (Ptr::CellStore *store);
            
            void unloadCell (CellRenderCollection::iterator iter);
            void loadCell (Ptr::CellStore *cell, MWRender::CellRender *render);
            void changeCell (int X, int Y, const ESM::Position& position, bool adjustPlayerPos);
            Ptr getPtr (const std::string& name, bool activeOnly);
            Ptr getPtrViaHandle (const std::string& handle);
            void playerCellChange (Ptr::CellStore *cell, const ESM::Position& position, bool adjustPlayerPos);
            void enable (Ptr reference);
            void disable (Ptr reference);
            void changeToInteriorCell (const std::string& cellName, const ESM::Position& position);
            void changeToExteriorCell (const ESM::Position& position);
            const ESM::Cell *getExterior (const std::string& cellName) const;
            void deleteObject (Ptr ptr);
            void moveObject (Ptr ptr, float x, float y, float z);
    };

}

#endif
