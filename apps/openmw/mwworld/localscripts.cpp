
#include "localscripts.hpp"

#include <components/esm_store/store.hpp>

#include "cellstore.hpp"

namespace
{
    template<typename T>
    void listCellScripts (MWWorld::LocalScripts& localScripts,
        MWWorld::CellRefList<T>& cellRefList,  MWWorld::Ptr::CellStore *cell)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter (
            cellRefList.list.begin());
            iter!=cellRefList.list.end(); ++iter)
        {
            if (!iter->base->mScript.empty() && iter->mData.getCount())
            {
                localScripts.add (iter->base->mScript, MWWorld::Ptr (&*iter, cell));
            }
        }
    }
}

MWWorld::LocalScripts::LocalScripts (const ESMS::ESMStore& store) : mStore (store) {}

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
    if (const ESM::Script *script = mStore.scripts.find (scriptName))
    {
        ptr.getRefData().setLocals (*script);

        mScripts.push_back (std::make_pair (scriptName, ptr));
    }
}

void MWWorld::LocalScripts::addCell (Ptr::CellStore *cell)
{
    listCellScripts (*this, cell->activators, cell);
    listCellScripts (*this, cell->potions, cell);
    listCellScripts (*this, cell->appas, cell);
    listCellScripts (*this, cell->armors, cell);
    listCellScripts (*this, cell->books, cell);
    listCellScripts (*this, cell->clothes, cell);
    listCellScripts (*this, cell->containers, cell);
    listCellScripts (*this, cell->creatures, cell);
    listCellScripts (*this, cell->doors, cell);
    listCellScripts (*this, cell->ingreds, cell);
    listCellScripts (*this, cell->lights, cell);
    listCellScripts (*this, cell->lockpicks, cell);
    listCellScripts (*this, cell->miscItems, cell);
    listCellScripts (*this, cell->npcs, cell);
    listCellScripts (*this, cell->probes, cell);
    listCellScripts (*this, cell->repairs, cell);
    listCellScripts (*this, cell->weapons, cell);
}

void MWWorld::LocalScripts::clear()
{
    mScripts.clear();
}

void MWWorld::LocalScripts::clearCell (Ptr::CellStore *cell)
{
    std::list<std::pair<std::string, Ptr> >::iterator iter = mScripts.begin();

    while (iter!=mScripts.end())
    {
        if (iter->second.getCell()==cell)
        {
            if (iter==mIter)
               ++mIter;

            mScripts.erase (iter++);
        }
        else
            ++iter;
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
