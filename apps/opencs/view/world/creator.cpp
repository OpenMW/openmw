
#include "creator.hpp"

#include <stdexcept>

CSVWorld::Creator::~Creator() {}

void CSVWorld::Creator::setScope (unsigned int scope)
{
    if (scope!=CSMWorld::Scope_Content)
        throw std::logic_error ("Invalid scope in creator");
}


CSVWorld::CreatorFactoryBase::~CreatorFactoryBase() {}


CSVWorld::Creator *CSVWorld::NullCreatorFactory::makeCreator (CSMWorld::Data& data,
    QUndoStack& undoStack, const CSMWorld::UniversalId& id) const
{
    return 0;
}
