
#include "world.hpp"

#include <cmath>
#include <iostream>

#include "components/bsa/bsa_archive.hpp"

#include "apps/openmw/mwrender/sky.hpp"
#include "apps/openmw/mwrender/interior.hpp"

#include "ptr.hpp"

namespace
{
    template<typename T>
    void listCellScripts (const ESMS::ESMStore& store,
        ESMS::CellRefList<T, MWWorld::RefData>& cellRefList, MWWorld::World::ScriptList& scriptList,
        MWWorld::Ptr::CellStore *cell)
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
                        std::make_pair (iter->base->script, MWWorld::Ptr (&*iter, cell)));
                }
            }
        }
    }
}

namespace MWWorld
{
    void World::insertInteriorScripts (ESMS::CellStore<RefData>& cell)
    {
        listCellScripts (mStore, cell.activators, mLocalScripts, &cell);
        listCellScripts (mStore, cell.potions, mLocalScripts, &cell);
        listCellScripts (mStore, cell.appas, mLocalScripts, &cell);
        listCellScripts (mStore, cell.armors, mLocalScripts, &cell);
        listCellScripts (mStore, cell.books, mLocalScripts, &cell);
        listCellScripts (mStore, cell.clothes, mLocalScripts, &cell);
        listCellScripts (mStore, cell.containers, mLocalScripts, &cell);
        listCellScripts (mStore, cell.creatures, mLocalScripts, &cell);
        listCellScripts (mStore, cell.doors, mLocalScripts, &cell);
        listCellScripts (mStore, cell.ingreds, mLocalScripts, &cell);
        listCellScripts (mStore, cell.lights, mLocalScripts, &cell);
        listCellScripts (mStore, cell.lockpicks, mLocalScripts, &cell);
        listCellScripts (mStore, cell.miscItems, mLocalScripts, &cell);
        listCellScripts (mStore, cell.npcs, mLocalScripts, &cell);
        listCellScripts (mStore, cell.probes, mLocalScripts, &cell);
        listCellScripts (mStore, cell.repairs, mLocalScripts, &cell);
        listCellScripts (mStore, cell.weapons, mLocalScripts, &cell);
    }
    
    Ptr World::getPtr (const std::string& name, Ptr::CellStore& cell)
    {
        if (ESMS::LiveCellRef<ESM::Activator, RefData> *ref = cell.activators.find (name))
            return Ptr (ref, &cell);

        if (ESMS::LiveCellRef<ESM::Potion, RefData> *ref = cell.potions.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Apparatus, RefData> *ref = cell.appas.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Armor, RefData> *ref = cell.armors.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Book, RefData> *ref = cell.books.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Clothing, RefData> *ref = cell.clothes.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Container, RefData> *ref = cell.containers.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Creature, RefData> *ref = cell.creatures.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Door, RefData> *ref = cell.doors.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Ingredient, RefData> *ref = cell.ingreds.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::CreatureLevList, RefData> *ref = cell.creatureLists.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::ItemLevList, RefData> *ref = cell.itemLists.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Light, RefData> *ref = cell.lights.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Tool, RefData> *ref = cell.lockpicks.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Misc, RefData> *ref = cell.miscItems.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::NPC, RefData> *ref = cell.npcs.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Tool, RefData> *ref = cell.probes.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Tool, RefData> *ref = cell.repairs.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Static, RefData> *ref = cell.statics.find (name))
            return Ptr (ref, &cell);
            
        if (ESMS::LiveCellRef<ESM::Weapon, RefData> *ref = cell.weapons.find (name))
            return Ptr (ref, &cell);
            
        return Ptr();
    }
    
    MWRender::CellRender *World::searchRender (Ptr::CellStore *store)
    {
        CellRenderCollection::iterator iter = mActiveCells.find (store);
        
        if (iter!=mActiveCells.end())
        {
            return iter->second;
        }
        else
        {
            iter = mBufferedCells.find (store);
            if (iter!=mBufferedCells.end())
                return iter->second;
        }
    
        return 0;
    }
    
    int World::getDaysPerMonth (int month) const
    {
        switch (month)
        {
            case 0: return 31;
            case 1: return 28;
            case 2: return 31;
            case 3: return 30;
            case 4: return 31;
            case 5: return 30;
            case 6: return 31;
            case 7: return 31;
            case 8: return 30;
            case 9: return 31;
            case 10: return 30;
            case 11: return 31;
        }
        
        throw std::runtime_error ("month out of range");
    }
    
    World::World (OEngine::Render::OgreRenderer& renderer, const boost::filesystem::path& dataDir,
        const std::string& master, bool newGame)
    : mSkyManager (0), mScene (renderer), mPlayerPos (0), mCurrentCell (0), mGlobalVariables (0),
      mSky (false), mCellChanged (false)
    {   
        boost::filesystem::path masterPath (dataDir);
        masterPath /= master;
        
        std::cout << "Loading ESM " << masterPath.string() << "\n";

        // This parses the ESM file and loads a sample cell
        mEsm.open (masterPath.file_string());
        mStore.load (mEsm);
        
        mPlayerPos = new MWRender::PlayerPos (mScene.getCamera(), mStore.npcs.find ("player"));

        // global variables
        mGlobalVariables = new Globals (mStore);
        
        if (newGame)
        {      
            // set new game mark
            mGlobalVariables->setInt ("chargenstate", 1);
        }
        
        mSkyManager =
            MWRender::SkyManager::create(renderer.getWindow(), mScene.getCamera());  
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
        delete mGlobalVariables;
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
        return mCellChanged;
    }
    
    Globals::Data& World::getGlobalVariable (const std::string& name)
    {
        return (*mGlobalVariables)[name];
    }
    
    char World::getGlobalVariableType (const std::string& name) const
    {
        return mGlobalVariables->getType (name);
    }    
    
    Ptr World::getPtr (const std::string& name, bool activeOnly)
    {
        // the player is always in an active cell.
        if (name=="player")
        {
            return mPlayerPos->getPlayer();
        }
        
        // active cells
        for (CellRenderCollection::iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
        {
            Ptr ptr = getPtr (name, *iter->first);
            
            if (!ptr.isEmpty())
                return ptr;
        }
        
        if (!activeOnly)
        {
            // TODO: inactive cells
        }
        
        throw std::runtime_error ("unknown ID: " + name);
    }
    
    void World::enable (Ptr reference)
    {
        if (!reference.getRefData().isEnabled())
        {
            reference.getRefData().enable();
            
            if (MWRender::CellRender *render = searchRender (reference.getCell()))
            {
                render->enable (reference.getRefData().getHandle());
            }
        }
    }
    
    void World::disable (Ptr reference)
    {
        if (!reference.getRefData().isEnabled())
        {
            reference.getRefData().enable();
            
            if (MWRender::CellRender *render = searchRender (reference.getCell()))
            {
                render->disable (reference.getRefData().getHandle());
            }
        }    
    }
    
    void World::advanceTime (double hours)
    {
        hours += mGlobalVariables->getFloat ("gamehour");

        setHour (hours);
        
        int days = hours / 24;
        
        if (days>0)
            mGlobalVariables->setInt ("dayspassed", days + mGlobalVariables->getInt ("dayspassed"));
    }
    
    void World::setHour (double hour)
    {
        if (hour<0)
            hour = 0;
    
        int days = hour / 24;
        
        hour = std::fmod (hour, 24);
     
        mGlobalVariables->setFloat ("gamehour", hour);
        
        mSkyManager->setHour (hour);
        
        if (days>0)
            setDay (days + mGlobalVariables->getInt ("day"));
    }
    
    void World::setDay (int day)
    {
        if (day<0)
            day = 0;

        int month = mGlobalVariables->getInt ("month");
    
        while (true)
        {
            int days = getDaysPerMonth (month);  
            if (day<days)
                break;
    
            if (month<11)
            {
                ++month;
            }
            else
            {
                month = 0;
                mGlobalVariables->setInt ("year", mGlobalVariables->getInt ("year")+1);
            }
            
            day -= days;
        }            
                          
        mGlobalVariables->setInt ("day", day);    
        mGlobalVariables->setInt ("month", month);

        mSkyManager->setDate (day, month);
    }   
    
    void World::setMonth (int month)
    {
        if (month<0)
            month = 0;
            
        int years = month / 12;
        month = month % 12;
        
        int days = getDaysPerMonth (month);
        
        if (mGlobalVariables->getInt ("day")>=days)
            mGlobalVariables->setInt ("day", days-1);
        
        mGlobalVariables->setInt ("month", month);
        
        if (years>0)
            mGlobalVariables->setInt ("year", years+mGlobalVariables->getInt ("year"));

        mSkyManager->setDate (mGlobalVariables->getInt ("day"), month);
    }
    
    void World::toggleSky()
    {
        if (mSky)
        {
            mSky = false;
            mSkyManager->disable();
        }
        else
        {
            mSky = true;
            // TODO check for extorior or interior with sky.
            mSkyManager->setHour (mGlobalVariables->getFloat ("gamehour"));
            mSkyManager->setDate (mGlobalVariables->getInt ("day"),
                mGlobalVariables->getInt ("month"));
            mSkyManager->enable();
        }
    }
    
    int World::getMasserPhase() const
    {
        return mSkyManager->getMasserPhase();
    }
    
    int World::getSecundaPhase() const
    {
        return mSkyManager->getSecundaPhase();
    }
    
    void World::setMoonColour (bool red)
    {
        mSkyManager->setMoonColour (red);   
    }
    
    float World::getTimeScaleFactor() const
    {
        return mGlobalVariables->getInt ("timescale");
    }
    
    void World::changeCell (const std::string& cellName, const ESM::Position& position)
    {
        // Load cell.       
        mInteriors[cellName].loadInt (cellName, mStore, mEsm);
     
        // remove active
        CellRenderCollection::iterator active = mActiveCells.begin();
        
        if (active!=mActiveCells.end())
        {
            active->second->destroy();
            delete active->second;
            mActiveCells.erase (active);
        }

        mLocalScripts.clear(); // FIXME won't work with exteriors        
        insertInteriorScripts (mInteriors[cellName]);

        mPlayerPos->setPos (position.pos[0], position.pos[1], position.pos[2]);
        mPlayerPos->setCell (&mInteriors[cellName]);
        // TODO orientation

        // This connects the cell data with the rendering scene.
        std::pair<CellRenderCollection::iterator, bool> result =
            mActiveCells.insert (std::make_pair (&mInteriors[cellName],
            new MWRender::InteriorCellRender (mInteriors[cellName], mScene)));

        if (result.second)
        {        
            // Load the cell and insert it into the renderer
            result.first->second->show();
        }

        if (mSky)
        {
            toggleSky();
            // TODO set weather
            toggleSky();
        }
        
        mCellChanged = true;
    }
    
    void World::markCellAsUnchanged()
    {
        mCellChanged = false;    
    }
}
