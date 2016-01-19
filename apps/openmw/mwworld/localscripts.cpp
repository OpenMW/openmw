#include "localscripts.hpp"

#include <iostream>

#include "esmstore.hpp"
#include "cellstore.hpp"

#include "class.hpp"
#include "containerstore.hpp"


namespace
{

    struct AddScriptsVisitor
    {
        AddScriptsVisitor(MWWorld::LocalScripts& scripts)
            : mScripts(scripts)
        {
        }
        MWWorld::LocalScripts& mScripts;

        bool operator()(const MWWorld::Ptr& ptr)
        {
            if (ptr.getRefData().isDeleted())
                return true;

            std::string script = ptr.getClass().getScript(ptr);

            if (!script.empty())
                mScripts.add(script, ptr);

            return true;
        }
    };

    struct AddContainerItemScriptsVisitor
    {
        AddContainerItemScriptsVisitor(MWWorld::LocalScripts& scripts)
            : mScripts(scripts)
        {
        }
        MWWorld::LocalScripts& mScripts;

        bool operator()(const MWWorld::Ptr& containerPtr)
        {
            MWWorld::ContainerStore& container = containerPtr.getClass().getContainerStore(containerPtr);
            for(MWWorld::ContainerStoreIterator it = container.begin(); it != container.end(); ++it)
            {
                std::string script = it->getClass().getScript(*it);
                if(script != "")
                {
                    MWWorld::Ptr item = *it;
                    item.mCell = containerPtr.getCell();
                    mScripts.add (script, item);
                }
            }
            return true;
        }
    };

}

MWWorld::LocalScripts::LocalScripts (const MWWorld::ESMStore& store) : mStore (store)
{
    mIter = mScripts.end();
}

void MWWorld::LocalScripts::setIgnore (const ConstPtr& ptr)
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
    if (const ESM::Script *script = mStore.get<ESM::Script>().search (scriptName))
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
    else
        std::cerr
            << "failed to add local script " << scriptName
            << " because the script does not exist." << std::endl;
}

void MWWorld::LocalScripts::addCell (CellStore *cell)
{
    AddScriptsVisitor addScriptsVisitor(*this);
    cell->forEach(addScriptsVisitor);

    AddContainerItemScriptsVisitor addContainerItemScriptsVisitor(*this);
    cell->forEachType<ESM::NPC>(addContainerItemScriptsVisitor);
    cell->forEachType<ESM::Creature>(addContainerItemScriptsVisitor);
    cell->forEachType<ESM::Container>(addContainerItemScriptsVisitor);
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
