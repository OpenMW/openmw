
#include "world.hpp"

#include "components/bsa/bsa_archive.hpp"
#include "components/engine/ogre/renderer.hpp"

#include "apps/openmw/mwrender/sky.hpp"
#include "apps/openmw/mwrender/interior.hpp"

namespace
{
    template<typename T>
    void listCellScripts (const ESMS::ESMStore& store,
        ESMS::CellRefList<T, OMW::RefData>& cellRefList, OMW::World::ScriptList& scriptList)
    {
        for (typename ESMS::CellRefList<T, OMW::RefData>::List::iterator iter (
            cellRefList.list.begin());
            iter!=cellRefList.list.end(); ++iter)
        {
            if (!iter->base->script.empty())
            {
                if (const ESM::Script *script = store.scripts.find (iter->base->script))
                {           
                    iter->mData.setLocals (*script);
            
                    scriptList.push_back (
                        std::make_pair (iter->base->script, &iter->mData.getLocals()));
                }
            }
        }
    }
}

namespace OMW
{
    void World::insertInteriorScripts (ESMS::CellStore<OMW::RefData>& cell)
    {
        listCellScripts (mStore, cell.activators, mLocalScripts);
        listCellScripts (mStore, cell.potions, mLocalScripts);
        listCellScripts (mStore, cell.appas, mLocalScripts);
        listCellScripts (mStore, cell.armors, mLocalScripts);
        listCellScripts (mStore, cell.books, mLocalScripts);
        listCellScripts (mStore, cell.clothes, mLocalScripts);
        listCellScripts (mStore, cell.containers, mLocalScripts);
        listCellScripts (mStore, cell.creatures, mLocalScripts);
        listCellScripts (mStore, cell.doors, mLocalScripts);
        listCellScripts (mStore, cell.ingreds, mLocalScripts);
        listCellScripts (mStore, cell.lights, mLocalScripts);
        listCellScripts (mStore, cell.lockpicks, mLocalScripts);
        listCellScripts (mStore, cell.miscItems, mLocalScripts);
        listCellScripts (mStore, cell.npcs, mLocalScripts);
        listCellScripts (mStore, cell.probes, mLocalScripts);
        listCellScripts (mStore, cell.repairs, mLocalScripts);
        listCellScripts (mStore, cell.weapons, mLocalScripts);
    }
    
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
        
        insertInteriorScripts (mInteriors[startCell]);
        
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
    
    ESMS::ESMStore& World::getStore()
    {
        return mStore;
    }
    
    const World::ScriptList& World::getLocalScripts() const
    {
        return mLocalScripts;
    }
}
