
#include "world.hpp"

#include "components/bsa/bsa_archive.hpp"
#include "components/engine/ogre/renderer.hpp"

#include "apps/openmw/mwrender/sky.hpp"
#include "apps/openmw/mwrender/interior.hpp"

namespace
{
    template<typename T>
    void listCellScripts (ESMS::CellRefList<T, OMW::RefData>& cellRefList,
        OMW::World::ScriptList& scriptList)
    {
        for (typename ESMS::CellRefList<T, OMW::RefData>::List::iterator iter (
            cellRefList.list.begin());
            iter!=cellRefList.list.end(); ++iter)
        {
            if (!iter->base->script.empty())
                scriptList.push_back (
                    std::make_pair (iter->base->script, static_cast<MWScript::Locals *> (0)));
            
            // TODO local variables
        
        
        }
    }
}

namespace OMW
{
    void World::insertInteriorScripts (ESMS::CellStore<OMW::RefData>& cell)
    {
        listCellScripts (cell.activators, mLocalScripts);
        listCellScripts (cell.potions, mLocalScripts);
        listCellScripts (cell.appas, mLocalScripts);
        listCellScripts (cell.armors, mLocalScripts);
        listCellScripts (cell.books, mLocalScripts);
        listCellScripts (cell.clothes, mLocalScripts);
        listCellScripts (cell.containers, mLocalScripts);
        listCellScripts (cell.creatures, mLocalScripts);
        listCellScripts (cell.doors, mLocalScripts);
        listCellScripts (cell.ingreds, mLocalScripts);
        listCellScripts (cell.lights, mLocalScripts);
        listCellScripts (cell.lockpicks, mLocalScripts);
        listCellScripts (cell.miscItems, mLocalScripts);
        listCellScripts (cell.npcs, mLocalScripts);
        listCellScripts (cell.probes, mLocalScripts);
        listCellScripts (cell.repairs, mLocalScripts);
        listCellScripts (cell.weapons, mLocalScripts);
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
