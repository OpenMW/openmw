#include "subviewfactory.hpp"

#include <cassert>

#include <stdexcept>

CSVDoc::SubViewFactoryBase::SubViewFactoryBase() {}

CSVDoc::SubViewFactoryBase::~SubViewFactoryBase() {}


CSVDoc::SubViewFactoryManager::SubViewFactoryManager() {}

CSVDoc::SubViewFactoryManager::~SubViewFactoryManager()
{
    for (std::map<CSMWorld::UniversalId::Type, SubViewFactoryBase *>::iterator iter (mSubViewFactories.begin());
        iter!=mSubViewFactories.end(); ++iter)
        delete iter->second;
}

void CSVDoc::SubViewFactoryManager::add (const CSMWorld::UniversalId::Type& id, SubViewFactoryBase *factory)
{
    assert (mSubViewFactories.find (id)==mSubViewFactories.end());

    mSubViewFactories.insert (std::make_pair (id, factory));
}

CSVDoc::SubView *CSVDoc::SubViewFactoryManager::makeSubView (const CSMWorld::UniversalId& id,
    CSMDoc::Document& document)
{
    std::map<CSMWorld::UniversalId::Type, SubViewFactoryBase *>::iterator iter = mSubViewFactories.find (id.getType());

    if (iter==mSubViewFactories.end())
        throw std::runtime_error ("Failed to create a sub view for: " + id.toString());

    return iter->second->makeSubView (id, document);
}
