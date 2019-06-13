#include "localscripts.hpp"

#include <components/debug/debuglog.hpp>

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
            // Ignore containers without generated content
            if (containerPtr.getTypeName() == typeid(ESM::Container).name() &&
                containerPtr.getRefData().getCustomData() == nullptr)
                return false;

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

void MWWorld::LocalScripts::startIteration()
{
    mIter = mScripts.begin();
}

bool MWWorld::LocalScripts::getNext(std::pair<std::string, Ptr>& script)
{
    while (mIter!=mScripts.end())
    {
        std::list<std::pair<std::string, Ptr> >::iterator iter = mIter++;
        script = *iter;
        return true;
    }
    return false;
}

void MWWorld::LocalScripts::add (const std::string& scriptName, const Ptr& ptr)
{
    if (const ESM::Script *script = mStore.get<ESM::Script>().search (scriptName))
    {
        try
        {
            ptr.getRefData().setLocals (*script);

            for (std::list<std::pair<std::string, Ptr> >::iterator iter = mScripts.begin(); iter!=mScripts.end(); ++iter)
                if (iter->second==ptr)
                {
                    Log(Debug::Warning) << "Error: tried to add local script twice for " << ptr.getCellRef().getRefId();
                    remove(ptr);
                    break;
                }

            mScripts.push_back (std::make_pair (scriptName, ptr));
        }
        catch (const std::exception& exception)
        {
            Log(Debug::Error)
                << "failed to add local script " << scriptName
                << " because an exception has been thrown: " << exception.what();
        }
    }
    else
        Log(Debug::Warning)
            << "failed to add local script " << scriptName
            << " because the script does not exist.";
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
