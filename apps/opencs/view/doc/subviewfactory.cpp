#include "subviewfactory.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <apps/opencs/model/world/universalid.hpp>

CSVDoc::SubViewFactoryManager::~SubViewFactoryManager()
{
    for (auto& entry : mSubViewFactories)
        delete entry.second;
}

void CSVDoc::SubViewFactoryManager::add(const CSMWorld::UniversalId::Type& id, SubViewFactoryBase* factory)
{
    assert(mSubViewFactories.find(id) == mSubViewFactories.end());

    mSubViewFactories.insert(std::make_pair(id, factory));
}

CSVDoc::SubView* CSVDoc::SubViewFactoryManager::makeSubView(const CSMWorld::UniversalId& id, CSMDoc::Document& document)
{
    std::map<CSMWorld::UniversalId::Type, SubViewFactoryBase*>::iterator iter = mSubViewFactories.find(id.getType());

    if (iter == mSubViewFactories.end())
        throw std::runtime_error("Failed to create a sub view for: " + id.toString());

    return iter->second->makeSubView(id, document);
}
