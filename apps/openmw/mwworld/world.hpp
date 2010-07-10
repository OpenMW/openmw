#ifndef GAME_MWWORLD_WORLD_H
#define GAME_MWWORLD_WORLD_H

#include <vector>
#include <map>

#include <boost/filesystem.hpp>

#include <components/esm_store/cell_store.hpp>

#include <components/interpreter/types.hpp>

#include "../mwrender/playerpos.hpp"
#include "../mwrender/mwscene.hpp"

#include "refdata.hpp"
#include "ptr.hpp"

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
            typedef ESMS::CellStore<RefData> CellStore;
        
        private:
        
            typedef std::map<CellStore *, MWRender::CellRender *> CellRenderCollection;
    
            MWRender::SkyManager* mSkyManager;
            MWRender::MWScene mScene;
            MWRender::PlayerPos *mPlayerPos;
            CellRenderCollection mActiveCells;
            CellRenderCollection mBufferedCells; // loaded, but not active (buffering not implementd yet)
            ESM::ESMReader mEsm;
            ESMS::ESMStore mStore;
            std::map<std::string, CellStore> mInteriors;
            ScriptList mLocalScripts;
            std::map<std::string, Interpreter::Type_Data> mGlobalVariables;
    
            // not implemented
            World (const World&);
            World& operator= (const World&);
    
            void insertInteriorScripts (ESMS::CellStore<RefData>& cell);
    
            Ptr getPtr (const std::string& name, CellStore& cellStore);
    
            MWRender::CellRender *searchRender (CellStore *store);
    
        public:
        
            World (Render::OgreRenderer& renderer, const boost::filesystem::path& master,
                const std::string& dataDir, const std::string& startCell, bool newGame);
                
            ~World();
            
            MWRender::PlayerPos& getPlayerPos();
            
            ESMS::ESMStore& getStore();
            
            const ScriptList& getLocalScripts() const;
            ///< Names and local variable state of all local scripts in active cells.
            
            bool hasCellChanged() const;
            ///< Has the player moved to a different cell, since the last frame?
            
            Interpreter::Type_Data& getGlobalVariable (const std::string& name);
            
            std::pair<Ptr, CellStore *> getPtr (const std::string& name, bool activeOnly);
            ///< Return a pointer to a liveCellRef with the given name.
            /// \param activeOnly do non search inactive cells.

            void enable (std::pair<Ptr, CellStore *>& reference);
            
            void disable (std::pair<Ptr, CellStore *>& reference);
            
            CellStore *find (const Ptr& ptr);
            ///< Only active cells are searched.
    };
}

#endif
