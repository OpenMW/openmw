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
    /// \brief The game world and its visual representation
    
    class World
    {
        public:
        
            typedef std::vector<std::pair<std::string, Ptr> > ScriptList;
        
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
            ScriptList mLocalScripts;
            MWWorld::Globals *mGlobalVariables;
    
            // not implemented
            World (const World&);
            World& operator= (const World&);
    
            void insertInteriorScripts (ESMS::CellStore<RefData>& cell);
    
            Ptr getPtr (const std::string& name, Ptr::CellStore& cellStore);
    
            MWRender::CellRender *searchRender (Ptr::CellStore *store);
    
        public:
        
           World (OEngine::Render::OgreRenderer& renderer, const boost::filesystem::path& master,
                const std::string& dataDir, const std::string& startCell, bool newGame);
                
            ~World();
            
            MWRender::PlayerPos& getPlayerPos();
            
            ESMS::ESMStore& getStore();
            
            const ScriptList& getLocalScripts() const;
            ///< Names and local variable state of all local scripts in active cells.
            
            bool hasCellChanged() const;
            ///< Has the player moved to a different cell, since the last frame?
            
            Globals::Data& getGlobalVariable (const std::string& name);
            
            Ptr getPtr (const std::string& name, bool activeOnly);
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            void enable (Ptr reference);
            
            void disable (Ptr reference);
    };
}

#endif
