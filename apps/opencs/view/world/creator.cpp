
#include "creator.hpp"

CSVWorld::Creator:: ~Creator() {}

CSVWorld::CreatorFactoryBase::~CreatorFactoryBase() {}


CSVWorld::Creator *CSVWorld::NullCreatorFactory::makeCreator (CSMWorld::Data& data,
    QUndoStack& undoStack, const CSMWorld::UniversalId& id) const
{
    return 0;
}