
#include "world.hpp"

#include <iostream>

#include "components/bsa/bsa_archive.hpp"
#include "components/engine/ogre/renderer.hpp"

#include "apps/openmw/mwrender/sky.hpp"
#include "apps/openmw/mwrender/interior.hpp"

namespace
{
    template<typename T>
    void listCellScripts (const ESMS::ESMStore& store,
        ESMS::CellRefList<T, MWWorld::RefData>& cellRefList, MWWorld::World::ScriptList& scriptList)
    {
        for (typename ESMS::CellRefList<T, MWWorld::RefData>::List::iterator iter (
            cellRefList.list.begin());
            iter!=cellRefList.list.end(); ++iter)
        {
            if (!iter->base->script.empty())
            {
                if (const ESM::Script *script = store.scripts.find (iter->base->script))
                {           
                    iter->mData.setLocals (*script);
            
                    scriptList.push_back (
                        std::make_pair (iter->base->script, MWWorld::Ptr (&*iter)));
                }
            }
        }
    }
}

namespace MWWorld
{
    void World::insertInteriorScripts (ESMS::CellStore<RefData>& cell)
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
    
    Ptr World::getPtr (const std::string& name, CellStore& cell)
    {
        if (ESMS::LiveCellRef<ESM::Activator, RefData> *ref = cell.activators.find (name))
            return ref;

        if (ESMS::LiveCellRef<ESM::Potion, RefData> *ref = cell.potions.find (name))
            return ref;
            
        if (ESMS::LiveCellRef<ESM::Apparatus, RefData> *ref = cell.appas.find (name))
            return ref;
            
        if (ESMS::LiveCellRef<ESM::Armor, RefData> *ref = cell.armors.find (name))
            return ref;
            
        if (ESMS::LiveCellRef<ESM::Book, RefData> *ref = cell.books.find (name))
            return ref;
            
        if (ESMS::LiveCellRef<ESM::Clothing, RefData> *ref = cell.clothes.find (name))
            return ref;
            
        if (ESMS::LiveCellRef<ESM::Container, RefData> *ref = cell.containers.find (name))
            return ref;
            
        if (ESMS::LiveCellRef<ESM::Creature, RefData> *ref = cell.creatures.find (name))
            return ref;
            
        if (ESMS::LiveCellRef<ESM::Door, RefData> *ref = cell.doors.find (name))
            return ref;
            
        if (ESMS::LiveCellRef<ESM::Ingredient, RefData> *ref = cell.ingreds.find (name))
            return ref;
            
        if (ESMS::LiveCellRef<ESM::CreatureLevList, RefData> *ref = cell.creatureLists.find (name))
            return ref;
            
        if (ESMS::LiveCellRef<ESM::ItemLevList, RefData> *ref = cell.itemLists.find (name))
            return ref;
            
        if (ESMS::LiveCellRef<ESM::Light, RefData> *ref = cell.lights.find (name))
            return ref;
            
        if (ESMS::LiveCellRef<ESM::Tool, RefData> *ref = cell.lockpicks.find (name))
            return ref;                
            
        if (ESMS::LiveCellRef<ESM::Misc, RefData> *ref = cell.miscItems.find (name))
            return ref;                
            
        if (ESMS::LiveCellRef<ESM::NPC, RefData> *ref = cell.npcs.find (name))
            return ref;                
            
        if (ESMS::LiveCellRef<ESM::Tool, RefData> *ref = cell.probes.find (name))
            return ref;                
            
        if (ESMS::LiveCellRef<ESM::Tool, RefData> *ref = cell.repairs.find (name))
            return ref;                
            
        if (ESMS::LiveCellRef<ESM::Static, RefData> *ref = cell.statics.find (name))
            return ref;                
            
        if (ESMS::LiveCellRef<ESM::Weapon, RefData> *ref = cell.weapons.find (name))
            return ref;                
            
        return Ptr();
    }
    
    World::World (Render::OgreRenderer& renderer, const boost::filesystem::path& dataDir,
        const std::string& master, const std::string& startCell, bool newGame)
    : mSkyManager (0), mScene (renderer), mPlayerPos (0)
    {   
        boost::filesystem::path masterPath (dataDir);
        masterPath /= master;
        
        std::cout << "Loading ESM " << masterPath.string() << "\n";

        // This parses the ESM file and loads a sample cell
        mEsm.open (masterPath.file_string());
        mStore.load (mEsm);
        
        mInteriors[startCell].loadInt (startCell, mStore, mEsm);
        
        insertInteriorScripts (mInteriors[startCell]);

        mPlayerPos = new MWRender::PlayerPos (mScene.getCamera(), mStore.npcs.find ("player"));

        // global variables
        for (ESMS::RecListT<ESM::Global>::MapType::const_iterator iter
            (mStore.globals.list.begin()); 
            iter != mStore.globals.list.end(); ++iter)
            mGlobalVariables.insert (std::make_pair (iter->first, iter->second.value));
        
        if (newGame)
        {      
            // set new game mark
            float newGameState = 1;
            mGlobalVariables["chargenstate"] =
                *reinterpret_cast<Interpreter::Type_Data *> (&newGameState);
        }
        
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
                        
        delete mPlayerPos;
        delete mSkyManager;
    }
    
    MWRender::PlayerPos& World::getPlayerPos()
    {
        return *mPlayerPos;
    }
    
    ESMS::ESMStore& World::getStore()
    {
        return mStore;
    }
    
    const World::ScriptList& World::getLocalScripts() const
    {
        return mLocalScripts;
    }
    
    bool World::hasCellChanged() const
    {
        // Cell change not implemented yet.
        return false;
    }
    
    Interpreter::Type_Data& World::getGlobalVariable (const std::string& name)
    {
        std::map<std::string, Interpreter::Type_Data>::iterator iter = mGlobalVariables.find (name);
        
        if (iter==mGlobalVariables.end())
            throw std::runtime_error ("unknown global variable: " + name);
            
        return iter->second;
    }
    
    std::pair<Ptr, World::CellStore *> World::getPtr (const std::string& name, bool activeOnly)
    {
        // the player is always in an active cell.
        if (name=="player")
        {
            // TODO: find real cell (might need to be stored in playerPos). For now we
            // use the first active cell. This will fail the moment we move into an
            // exterior cell.
            return std::make_pair (mPlayerPos->getPlayer(), mActiveCells.begin()->first);
        }
        
        // active cells
        for (CellRenderCollection::iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
        {
            Ptr ptr = getPtr (name, *iter->first);
            
            if (!ptr.isEmpty())
                return std::make_pair (ptr, iter->first);
        }
        
        if (!activeOnly)
        {
            // TODO: inactive cells
        }
        
        throw std::runtime_error ("unknown ID: " + name);
    }
}
