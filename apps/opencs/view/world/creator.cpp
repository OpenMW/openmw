
#include "creator.hpp"

CSVWorld::Creator:: ~Creator() {}

CSVWorld::CreatorFactoryBase::~CreatorFactoryBase() {}


CSVWorld::Creator *CSVWorld::NullCreatorFactory::makeCreator() const
{
    return 0;
}