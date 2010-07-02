#ifndef WORLD_H
#define WORLD_H

#include <vector>
#include <map>

#include <boost/filesystem.hpp>

#include "components/esm_store/cell_store.hpp"

#include "apps/openmw/mwrender/playerpos.hpp"
#include "apps/openmw/mwrender/mwscene.hpp"

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
            typedef std::map<ESMS::CellStore *, MWRender::CellRender *> CellRenderCollection;
    
            MWRender::SkyManager* mSkyManager;
            MWRender::MWScene mScene;
            MWRender::PlayerPos mPlayerPos;
            CellRenderCollection mActiveCells;
            CellRenderCollection mBufferedCells; // loaded, but not active (buffering not implementd yet)
            ESM::ESMReader mEsm;
            ESMS::ESMStore mStore;
            std::map<std::string, ESMS::CellStore> mInteriors;
    
            // not implemented
            World (const World&);
            World& operator= (const World&);
    
        public:
        
            World (Render::OgreRenderer& renderer, const boost::filesystem::path& master,
                const std::string& dataDir, const std::string& startCell);
                
            ~World();
            
            MWRender::PlayerPos& getPlayerPos();
    
    
    };
}

#endif
