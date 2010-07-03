#ifndef WORLD_H
#define WORLD_H

#include <vector>
#include <map>

#include <boost/filesystem.hpp>

#include "components/esm_store/cell_store.hpp"

#include "apps/openmw/mwrender/playerpos.hpp"
#include "apps/openmw/mwrender/mwscene.hpp"

#include "refdata.hpp"

namespace Render
{
    class OgreRenderer;
}

namespace MWRender
{
    class SkyManager;
    class CellRender;
}
 
namespace OMW
{
    /// \brief The game world and its visual representation
    
    class World
    {
        public:
        
            typedef std::vector<std::pair<std::string, MWScript::Locals *> > ScriptList;
        
        private:
        
            typedef ESMS::CellStore<RefData> CellStore;
            typedef std::map<CellStore *, MWRender::CellRender *> CellRenderCollection;
    
            MWRender::SkyManager* mSkyManager;
            MWRender::MWScene mScene;
            MWRender::PlayerPos mPlayerPos;
            CellRenderCollection mActiveCells;
            CellRenderCollection mBufferedCells; // loaded, but not active (buffering not implementd yet)
            ESM::ESMReader mEsm;
            ESMS::ESMStore mStore;
            std::map<std::string, CellStore> mInteriors;
            ScriptList mLocalScripts;
    
            // not implemented
            World (const World&);
            World& operator= (const World&);
    
            void insertInteriorScripts (ESMS::CellStore<OMW::RefData>& cell);
    
        public:
        
            World (Render::OgreRenderer& renderer, const boost::filesystem::path& master,
                const std::string& dataDir, const std::string& startCell);
                
            ~World();
            
            MWRender::PlayerPos& getPlayerPos();
            
            ESMS::ESMStore& getStore();
            
            const ScriptList& getLocalScripts() const;
            ///< Names and local variable state of all local scripts in active cells.
            
            bool hasCellChanged() const;
            ///< Has the player moved to a different cell, since the last frame?
    };
}

#endif
