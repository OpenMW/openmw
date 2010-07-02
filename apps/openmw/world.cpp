
#include "world.hpp"

#include "components/bsa/bsa_archive.hpp"
#include "components/engine/ogre/renderer.hpp"

#include "apps/openmw/mwrender/sky.hpp"
#include "apps/openmw/mwrender/interior.hpp"

namespace OMW
{
    World::World (Render::OgreRenderer& renderer, const boost::filesystem::path& dataDir,
        const std::string& master, const std::string& startCell)
    : mSkyManager (0), mScene (renderer), mPlayerPos (mScene.getCamera())
    {   
        boost::filesystem::path masterPath (dataDir);
        masterPath /= master;
        
        std::cout << "Loading ESM " << masterPath.string() << "\n";

        // This parses the ESM file and loads a sample cell
        mEsm.open (masterPath.file_string());
        mStore.load (mEsm);
        
        mInteriors[startCell].loadInt (startCell, mStore, mEsm);
        
        std::cout << "\nSetting up cell rendering\n";

        // This connects the cell data with the rendering scene.
        mActiveCells.insert (std::make_pair (&mInteriors[startCell],
            new MWRender::InteriorCellRender (mInteriors[startCell], mScene)));
        
        // Load the cell and insert it into the renderer
        for (CellRenderCollection::iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
            iter->second->show();

        // Optionally enable the sky
//        if (mEnableSky)
//            mpSkyManager = MWRender::SkyManager::create(renderer.getWindow(), mScene.getCamera());
        
    }
    
    World::~World()
    {
        for (CellRenderCollection::iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
            delete iter->second;
            
        for (CellRenderCollection::iterator iter (mBufferedCells.begin());
            iter!=mBufferedCells.end(); ++iter)
            delete iter->second;
                        
        delete mSkyManager;
    }
    
    MWRender::PlayerPos& World::getPlayerPos()
    {
        return mPlayerPos;
    }
}
