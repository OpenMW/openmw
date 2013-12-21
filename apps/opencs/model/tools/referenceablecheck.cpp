#include "referenceablecheck.hpp"

#include <sstream>
#include <map>
#include <cassert>

#include "../world/record.hpp"

#include "../world/universalid.hpp"

CSMTools::ReferenceableCheckStage::ReferenceableCheckStage(const CSMWorld::RefIdData& referenceable) :
    mReferencables(referenceable),
    mBooksSize(0),
    mActivatorsSize(0)
{
    setSizeVariables();
}

void CSMTools::ReferenceableCheckStage::perform(int stage, std::vector< std::string >& messages)
{
    //Checks for books, than, when stage is above mBooksSize goes to other checks, with (stage - PrevSum) as stage.
    bool CheckPerformed = false;
    int PrevSum(0);

    if (stage <= mBooksSize)
    {
        bookCheck(stage, mReferencables.getBooks(), messages);
        CheckPerformed = true;
    }

    if (CheckPerformed)
    {
        return;
    }

    PrevSum += mBooksSize;

    if ((stage - PrevSum) <= mActivatorsSize)
    {
        activatorCheck(stage - PrevSum, mReferencables.getActivators(), messages);
        CheckPerformed = true;
    }

    if (CheckPerformed)
    {
        return;
    }

    PrevSum += mActivatorsSize;
}

int CSMTools::ReferenceableCheckStage::setup()
{
    return mReferencables.getSize();
}

void CSMTools::ReferenceableCheckStage::bookCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Book >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Book& Book = (static_cast<const CSMWorld::Record<ESM::Book>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Book, Book.mId);

    //Checking for name
    if (Book.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Book.mId + " has an empty name");
    }

    //Checking for weight
    if (Book.mData.mWeight < 0)
    {
        messages.push_back(id.toString() + "|" + Book.mId + " has a negative weight");
    }

    //Checking for value
    if (Book.mData.mValue < 0)
    {
        messages.push_back(id.toString() + "|" + Book.mId + " has a negative value");
    }

//checking for model
    if (Book.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Book.mId + " has no model");
    }

    //checking for icon
    if (Book.mIcon.empty())
    {
        messages.push_back(id.toString() + "|" + Book.mId + " has no icon");
    }
}

void CSMTools::ReferenceableCheckStage::activatorCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Activator >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Activator& Activator = (static_cast<const CSMWorld::Record<ESM::Activator>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Activator, Activator.mId);

    //Checking for model, IIRC all activators should have a model
    if (Activator.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Activator.mId + " has no model");
    }
}

void CSMTools::ReferenceableCheckStage::setSizeVariables()
{
    mBooksSize = mReferencables.getBooks().getSize();
    mActivatorsSize = mReferencables.getActivator().getSize();
}
