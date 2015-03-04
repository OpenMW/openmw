#include "localscripts.hpp"

#include <iostream>

#include "esmstore.hpp"
#include "cellstore.hpp"

#include "class.hpp"
#include "containerstore.hpp"


namespace
{
    template<typename T>
    void listCellScripts (MWWorld::LocalScripts& localScripts,
        MWWorld::CellRefList<T>& cellRefList,  MWWorld::CellStore *cell)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter (
            cellRefList.mList.begin());
            iter!=cellRefList.mList.end(); ++iter)
        {
            if (!iter->mBase->mScript.empty() && !iter->mData.isDeleted())
            {
                localScripts.add (iter->mBase->mScript, MWWorld::Ptr (&*iter, cell));
            }
        }
    }

    // Adds scripts for items in containers (containers/npcs/creatures)
    template<typename T>
    void listCellScriptsCont (MWWorld::LocalScripts& localScripts,
        MWWorld::CellRefList<T>& cellRefList,  MWWorld::CellStore *cell)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter (
            cellRefList.mList.begin());
            iter!=cellRefList.mList.end(); ++iter)
        {

            MWWorld::Ptr containerPtr (&*iter, cell);

            MWWorld::ContainerStore& container = containerPtr.getClass().getContainerStore(containerPtr);
            for(MWWorld::ContainerStoreIterator it3 = container.begin(); it3 != container.end(); ++it3)
            {
                std::string script = it3->getClass().getScript(*it3);
                if(script != "")
                {
                    MWWorld::Ptr item = *it3;
                    item.mCell = cell;
                    localScripts.add (script, item);
                }
            }
        }
    }
}

MWWorld::LocalScripts::LocalScripts (const MWWorld::ESMStore& store) : mStore (store) {}

void MWWorld::LocalScripts::setIgnore (const Ptr& ptr)
{
    mIgnore = ptr;
}

void MWWorld::LocalScripts::startIteration()
{
    mIter = mScripts.begin();
}

bool MWWorld::LocalScripts::isFinished() const
{
    if (mIter==mScripts.end())
        return true;

    if (!mIgnore.isEmpty() && mIter->second==mIgnore)
    {
        std::list<std::pair<std::string, Ptr> >::iterator iter = mIter;
        return ++iter==mScripts.end();
    }

    return false;
}

std::pair<std::string, MWWorld::Ptr> MWWorld::LocalScripts::getNext()
{
    assert (!isFinished());

    std::list<std::pair<std::string, Ptr> >::iterator iter = mIter++;

    if (mIgnore.isEmpty() || iter->second!=mIgnore)
        return *iter;

    return getNext();
}

void MWWorld::LocalScripts::add (const std::string& scriptName, const Ptr& ptr)
{
    if (const ESM::Script *script = mStore.get<ESM::Script>().find (scriptName))
    {
        try
        {
            ptr.getRefData().setLocals (*script);

            mScripts.push_back (std::make_pair (scriptName, ptr));
        }
        catch (const std::exception& exception)
        {
            std::cerr
                << "failed to add local script " << scriptName
                << " because an exception has been thrown: " << exception.what() << std::endl;
        }
    }
}

void MWWorld::LocalScripts::addCell (CellStore *cell)
{
    listCellScripts (*this, cell->get<ESM::Activator>(), cell);
    listCellScripts (*this, cell->get<ESM::Potion>(), cell);
    listCellScripts (*this, cell->get<ESM::Apparatus>(), cell);
    listCellScripts (*this, cell->get<ESM::Armor>(), cell);
    listCellScripts (*this, cell->get<ESM::Book>(), cell);
    listCellScripts (*this, cell->get<ESM::Clothing>(), cell);
    listCellScripts (*this, cell->get<ESM::Container>(), cell);
    listCellScriptsCont (*this, cell->get<ESM::Container>(), cell);
    listCellScripts (*this, cell->get<ESM::Creature>(), cell);
    listCellScriptsCont (*this, cell->get<ESM::Creature>(), cell);
    listCellScripts (*this, cell->get<ESM::Door>(), cell);
    listCellScripts (*this, cell->get<ESM::Ingredient>(), cell);
    listCellScripts (*this, cell->get<ESM::Light>(), cell);
    listCellScripts (*this, cell->get<ESM::Lockpick>(), cell);
    listCellScripts (*this, cell->get<ESM::Miscellaneous>(), cell);
    listCellScripts (*this, cell->get<ESM::NPC>(), cell);
    listCellScriptsCont (*this, cell->get<ESM::NPC>(), cell);
    listCellScripts (*this, cell->get<ESM::Probe>(), cell);
    listCellScripts (*this, cell->get<ESM::Repair>(), cell);
    listCellScripts (*this, cell->get<ESM::Weapon>(), cell);
}

void MWWorld::LocalScripts::clear()
{
    mScripts.clear();
}

void MWWorld::LocalScripts::clearCell (CellStore *cell)
{
    std::list<std::pair<std::string, Ptr> >::iterator iter = mScripts.begin();

    while (iter!=mScripts.end())
    {
        if (iter->second.mCell==cell)
        {
            if (iter==mIter)
               ++mIter;

            mScripts.erase (iter++);
        }
        else
            ++iter;
    }
}

void MWWorld::LocalScripts::remove (RefData *ref)
{
    for (std::list<std::pair<std::string, Ptr> >::iterator iter = mScripts.begin();
        iter!=mScripts.end(); ++iter)
        if (&(iter->second.getRefData()) == ref)
        {
            if (iter==mIter)
                ++mIter;

            mScripts.erase (iter);
            break;
        }
}

void MWWorld::LocalScripts::remove (const Ptr& ptr)
{
    for (std::list<std::pair<std::string, Ptr> >::iterator iter = mScripts.begin();
        iter!=mScripts.end(); ++iter)
        if (iter->second==ptr)
        {
            if (iter==mIter)
                ++mIter;

            mScripts.erase (iter);
            break;
        }
}
