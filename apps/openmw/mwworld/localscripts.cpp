#include "localscripts.hpp"

#include "esmstore.hpp"
#include "cellstore.hpp"

namespace
{
    template<typename T>
    void listCellScripts (MWWorld::LocalScripts& localScripts,
        MWWorld::CellRefList<T>& cellRefList,  MWWorld::Ptr::CellStore *cell)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter (
            cellRefList.mList.begin());
            iter!=cellRefList.mList.end(); ++iter)
        {
            if (!iter->mBase->mScript.empty() && iter->mData.getCount())
            {
                localScripts.add (iter->mBase->mScript, MWWorld::Ptr (&*iter, cell));
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
        ptr.getRefData().setLocals (*script);

        mScripts.push_back (std::make_pair (scriptName, ptr));
    }
}

void MWWorld::LocalScripts::addCell (Ptr::CellStore *cell)
{
    listCellScripts (*this, cell->mActivators, cell);
    listCellScripts (*this, cell->mPotions, cell);
    listCellScripts (*this, cell->mAppas, cell);
    listCellScripts (*this, cell->mArmors, cell);
    listCellScripts (*this, cell->mBooks, cell);
    listCellScripts (*this, cell->mClothes, cell);
    listCellScripts (*this, cell->mContainers, cell);
    listCellScripts (*this, cell->mCreatures, cell);
    listCellScripts (*this, cell->mDoors, cell);
    listCellScripts (*this, cell->mIngreds, cell);
    listCellScripts (*this, cell->mLights, cell);
    listCellScripts (*this, cell->mLockpicks, cell);
    listCellScripts (*this, cell->mMiscItems, cell);
    listCellScripts (*this, cell->mNpcs, cell);
    listCellScripts (*this, cell->mProbes, cell);
    listCellScripts (*this, cell->mRepairs, cell);
    listCellScripts (*this, cell->mWeapons, cell);
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
