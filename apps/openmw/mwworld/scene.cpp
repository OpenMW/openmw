#include "scene.hpp"
#include "world.hpp"

#include "../mwbase/environment.hpp"

#include "../mwmechanics/mechanicsmanager.hpp"

#include "../mwsound/soundmanager.hpp"

#include "../mwgui/window_manager.hpp"

#include "ptr.hpp"
#include "player.hpp"
#include "class.hpp"

#include "cellfunctors.hpp"

namespace
{

    template<typename T>
    void insertCellRefList(MWRender::RenderingManager& rendering,
        T& cellRefList, ESMS::CellStore<MWWorld::RefData> &cell, MWWorld::PhysicsSystem& physics)
    {
        if (!cellRefList.list.empty())
        {
            const MWWorld::Class& class_ =
                MWWorld::Class::get (MWWorld::Ptr (&*cellRefList.list.begin(), &cell));

            for (typename T::List::iterator it = cellRefList.list.begin();
                it != cellRefList.list.end(); it++)
            {
                if (it->mData.getCount() || it->mData.isEnabled())
                {
                    MWWorld::Ptr ptr (&*it, &cell);

                    try
                    {
                        rendering.addObject(ptr);
                        class_.insertObject(ptr, physics);
                        class_.enable (ptr);
                    }
                    catch (const std::exception& e)
                    {
                        std::string error ("error during rendering: ");
                        std::cerr << error + e.what() << std::endl;
                    }
                }
            }
        }
    }

}


namespace MWWorld
{

    void Scene::update (float duration){
        mRendering.update (duration);
    }

    void Scene::unloadCell (CellStoreCollection::iterator iter)
    {
        std::cout << "Unloading cell\n";
        ListHandles functor;






        (*iter)->forEach<ListHandles>(functor);

        {


            // silence annoying g++ warning
            for (std::vector<Ogre::SceneNode*>::const_iterator iter2 (functor.mHandles.begin());
                iter2!=functor.mHandles.end(); ++iter2){
                 Ogre::SceneNode* node = *iter2;
                mPhysics->removeObject (node->getName());
            }

            if (!((*iter)->cell->data.flags & ESM::Cell::Interior))
                mPhysics->removeHeightField( (*iter)->cell->data.gridX, (*iter)->cell->data.gridY );
        }

		mRendering.removeCell(*iter);
		//mPhysics->removeObject("Unnamed_43");

        mWorld->getLocalScripts().clearCell (*iter);
        MWBase::Environment::get().getMechanicsManager()->dropActors (*iter);
        MWBase::Environment::get().getSoundManager()->stopSound (*iter);
		mActiveCells.erase(*iter);



    }

    void Scene::loadCell (Ptr::CellStore *cell)
    {
        // register local scripts
        mWorld->getLocalScripts().addCell (cell);



        std::pair<CellStoreCollection::iterator, bool> result =
            mActiveCells.insert(cell);

        if(result.second)
        {
            insertCell(*cell);
            mRendering.cellAdded (cell);

            float verts = ESM::Land::LAND_SIZE;
            float worldsize = ESM::Land::REAL_SIZE;

            if (!(cell->cell->data.flags & ESM::Cell::Interior))
            {
                ESM::Land* land = mWorld->getStore().lands.search(cell->cell->data.gridX,cell->cell->data.gridY);
                mPhysics->addHeightField (land->landData->heights,
                    cell->cell->data.gridX, cell->cell->data.gridY,
                    0, ( worldsize/(verts-1) ), verts);
            }

            mRendering.configureAmbient(*cell);
            mRendering.requestMap(cell);
            mRendering.configureAmbient(*cell);

        }

    }

    void Scene::playerCellChange (Ptr::CellStore *cell, const ESM::Position& position,
        bool adjustPlayerPos)
    {
        bool hasWater = cell->cell->data.flags & cell->cell->HasWater;
        mPhysics->setCurrentWater(hasWater, cell->cell->water);
        if (adjustPlayerPos)
            mWorld->getPlayer().setPos (position.pos[0], position.pos[1], position.pos[2]);

        mWorld->getPlayer().setCell (cell);
        // TODO orientation
        MWBase::Environment::get().getMechanicsManager()->addActor (mWorld->getPlayer().getPlayer());
        MWBase::Environment::get().getMechanicsManager()->watchActor (mWorld->getPlayer().getPlayer());

        MWBase::Environment::get().getWindowManager()->changeCell( mCurrentCell );
    }

    void Scene::changeCell (int X, int Y, const ESM::Position& position, bool adjustPlayerPos)
    {
        mRendering.preCellChange(mCurrentCell);

        // remove active
        MWBase::Environment::get().getMechanicsManager()->removeActor (mWorld->getPlayer().getPlayer());

        CellStoreCollection::iterator active = mActiveCells.begin();

        while (active!=mActiveCells.end())
        {
            if (!((*active)->cell->data.flags & ESM::Cell::Interior))
            {
                if (std::abs (X-(*active)->cell->data.gridX)<=1 &&
                    std::abs (Y-(*active)->cell->data.gridY)<=1)
                {
                    // keep cells within the new 3x3 grid
                    ++active;
                    continue;
                }
            }

            unloadCell (active++);
        }

        // Load cells
        for (int x=X-1; x<=X+1; ++x)
            for (int y=Y-1; y<=Y+1; ++y)
            {
                CellStoreCollection::iterator iter = mActiveCells.begin();

                while (iter!=mActiveCells.end())
                {
                    assert (!((*iter)->cell->data.flags & ESM::Cell::Interior));

                    if (x==(*iter)->cell->data.gridX &&
                        y==(*iter)->cell->data.gridY)
                        break;

                    ++iter;
                }

                if (iter==mActiveCells.end())
                {
                    Ptr::CellStore *cell = mWorld->getExterior(x, y);

                    loadCell (cell);
                }
            }

        // find current cell
        CellStoreCollection::iterator iter = mActiveCells.begin();

        while (iter!=mActiveCells.end())
        {
            assert (!((*iter)->cell->data.flags & ESM::Cell::Interior));

            if (X==(*iter)->cell->data.gridX &&
                Y==(*iter)->cell->data.gridY)
                break;

            ++iter;
        }

        assert (iter!=mActiveCells.end());

        mCurrentCell = *iter;


        // adjust player
        playerCellChange (mWorld->getExterior(X, Y), position, adjustPlayerPos);

        // Sky system
        mWorld->adjustSky();

        mRendering.switchToExterior();

        mCellChanged = true;
    }

    //We need the ogre renderer and a scene node.
    Scene::Scene (World *world, MWRender::RenderingManager& rendering, PhysicsSystem *physics)
    : mCurrentCell (0), mCellChanged (false), mWorld(world),
      mPhysics(physics), mRendering(rendering)
    {
    }

    Scene::~Scene()
    {
    }

    bool Scene::hasCellChanged() const
    {
        return mCellChanged;
    }

    const Scene::CellStoreCollection& Scene::getActiveCells() const
    {
        return mActiveCells;
    }

    void Scene::changeToInteriorCell (const std::string& cellName, const ESM::Position& position)
    {
        std::cout << "Changing to interior\n";
        // remove active
        CellStoreCollection::iterator active = mActiveCells.begin();

        while (active!=mActiveCells.end())
        {
            unloadCell (active++);
        }

        // Load cell.
        std::cout << "cellName:" << cellName << std::endl;
        Ptr::CellStore *cell = mWorld->getInterior(cellName);

        loadCell (cell);


        // adjust player
        mCurrentCell = cell;
        playerCellChange (cell, position);

        // adjust fog
        mRendering.switchToInterior();
        mRendering.configureFog(*cell);

        // Sky system
        mWorld->adjustSky();

        mCellChanged = true;
    }

    void Scene::changeToExteriorCell (const ESM::Position& position)
    {
        int x = 0;
        int y = 0;

        mWorld->positionToIndex (position.pos[0], position.pos[1], x, y);

        changeCell (x, y, position, true);
    }

    Ptr::CellStore* Scene::getCurrentCell ()
    {
        return mCurrentCell;
    }

    void Scene::markCellAsUnchanged()
    {
        mCellChanged = false;
    }

    void Scene::insertCell(ESMS::CellStore<MWWorld::RefData> &cell)
    {
        // Loop through all references in the cell
        insertCellRefList(mRendering, cell.activators, cell, *mPhysics);
        insertCellRefList(mRendering, cell.potions, cell, *mPhysics);
        insertCellRefList(mRendering, cell.appas, cell, *mPhysics);
        insertCellRefList(mRendering, cell.armors, cell, *mPhysics);
        insertCellRefList(mRendering, cell.books, cell, *mPhysics);
        insertCellRefList(mRendering, cell.clothes, cell, *mPhysics);
        insertCellRefList(mRendering, cell.containers, cell, *mPhysics);
        insertCellRefList(mRendering, cell.creatures, cell, *mPhysics);
        insertCellRefList(mRendering, cell.doors, cell, *mPhysics);
        insertCellRefList(mRendering, cell.ingreds, cell, *mPhysics);
        insertCellRefList(mRendering, cell.creatureLists, cell, *mPhysics);
        insertCellRefList(mRendering, cell.itemLists, cell, *mPhysics);
        insertCellRefList(mRendering, cell.lights, cell, *mPhysics);
        insertCellRefList(mRendering, cell.lockpicks, cell, *mPhysics);
        insertCellRefList(mRendering, cell.miscItems, cell, *mPhysics);
        insertCellRefList(mRendering, cell.npcs, cell, *mPhysics);
        insertCellRefList(mRendering, cell.probes, cell, *mPhysics);
        insertCellRefList(mRendering, cell.repairs, cell, *mPhysics);
        insertCellRefList(mRendering, cell.statics, cell, *mPhysics);
        insertCellRefList(mRendering, cell.weapons, cell, *mPhysics);
    }

    void Scene::insertObject(MWWorld::Ptr ptr, Ptr::CellStore* cell)
    {
        std::string type = ptr.getTypeName();

        MWWorld::Ptr newPtr;

        // insert into the correct CellRefList
        if      (type == typeid(ESM::Potion).name())
        {
            ESMS::LiveCellRef<ESM::Potion, MWWorld::RefData>* ref = ptr.get<ESM::Potion>();
            newPtr = MWWorld::Ptr(ref, cell);
            cell->potions.list.push_back( *ref );
        }
        else if (type == typeid(ESM::Apparatus).name())
        {
            ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData>* ref = ptr.get<ESM::Apparatus>();
            newPtr = MWWorld::Ptr(ref, cell);
            cell->appas.list.push_back( *ref );
        }
        else if (type == typeid(ESM::Armor).name())
        {
            ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData>* ref = ptr.get<ESM::Armor>();
            newPtr = MWWorld::Ptr(ref, cell);
            cell->armors.list.push_back( *ref );
        }
        else if (type == typeid(ESM::Book).name())
        {
            ESMS::LiveCellRef<ESM::Book, MWWorld::RefData>* ref = ptr.get<ESM::Book>();
            newPtr = MWWorld::Ptr(ref, cell);
            cell->books.list.push_back( *ref );
        }
        else if (type == typeid(ESM::Clothing).name())
        {
            ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData>* ref = ptr.get<ESM::Clothing>();
            newPtr = MWWorld::Ptr(ref, cell);
            cell->clothes.list.push_back( *ref );
        }
        else if (type == typeid(ESM::Ingredient).name())
        {
            ESMS::LiveCellRef<ESM::Ingredient, MWWorld::RefData>* ref = ptr.get<ESM::Ingredient>();
            newPtr = MWWorld::Ptr(ref, cell);
            cell->ingreds.list.push_back( *ref );
        }
        else if (type == typeid(ESM::Light).name())
        {
            ESMS::LiveCellRef<ESM::Light, MWWorld::RefData>* ref = ptr.get<ESM::Light>();
            newPtr = MWWorld::Ptr(ref, cell);
            cell->lights.list.push_back( *ref );
        }
        else if (type == typeid(ESM::Tool).name())
        {
            ESMS::LiveCellRef<ESM::Tool, MWWorld::RefData>* ref = ptr.get<ESM::Tool>();
            newPtr = MWWorld::Ptr(ref, cell);
            cell->lockpicks.list.push_back( *ref );
        }
        else if (type == typeid(ESM::Repair).name())
        {
            ESMS::LiveCellRef<ESM::Repair, MWWorld::RefData>* ref = ptr.get<ESM::Repair>();
            newPtr = MWWorld::Ptr(ref, cell);
            cell->repairs.list.push_back( *ref );
        }
        else if (type == typeid(ESM::Probe).name())
        {
            ESMS::LiveCellRef<ESM::Probe, MWWorld::RefData>* ref = ptr.get<ESM::Probe>();
            newPtr = MWWorld::Ptr(ref, cell);
            cell->probes.list.push_back( *ref );
        }
        else if (type == typeid(ESM::Weapon).name())
        {
            ESMS::LiveCellRef<ESM::Weapon, MWWorld::RefData>* ref = ptr.get<ESM::Weapon>();
            newPtr = MWWorld::Ptr(ref, cell);
            cell->weapons.list.push_back( *ref );
        }
        else if (type == typeid(ESM::Miscellaneous).name())
        {
            ESMS::LiveCellRef<ESM::Miscellaneous, MWWorld::RefData>* ref = ptr.get<ESM::Miscellaneous>();
            cell->miscItems.list.push_back( *ref );
            newPtr = MWWorld::Ptr(&cell->miscItems.list.back(), cell);
        }
        else
            throw std::runtime_error("Trying to insert object of unhandled type");



        newPtr.getRefData().setCount(ptr.getRefData().getCount());
        ptr.getRefData().setCount(0);
        newPtr.getRefData().enable();

        mRendering.addObject(newPtr);
        MWWorld::Class::get(newPtr).insertObject(newPtr, *mPhysics);
        MWWorld::Class::get(newPtr).enable(newPtr);

    }

}
