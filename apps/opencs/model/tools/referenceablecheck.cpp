#include "referenceablecheck.hpp"

#include <sstream>
#include <map>
#include <cassert>

#include <components/esm/loadbook.hpp>
#include "../world/record.hpp"

#include "../world/universalid.hpp"

CSMTools::ReferenceableCheckStage::ReferenceableCheckStage(const CSMWorld::RefIdData& referenceable) :
    mReferencables(referenceable),
    mBooksSize(0)
{
    setSizeVariables();
}

void CSMTools::ReferenceableCheckStage::perform(int stage, std::vector< std::string >& messages)
{
    //Checks for books, than, when stage is above mBooksSize goes to other checks, with stage - minus prev sizes as stage.
    bool CheckPerformed = false;
    
    if (stage <= mBooksSize)
    {	
	bookCheck(stage, mReferencables.getBooks(), messages);
        CheckPerformed = true;
    }

    if (CheckPerformed)
    {
        return;
    }
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
}

void CSMTools::ReferenceableCheckStage::setSizeVariables()
{
    mBooksSize = mReferencables.getBooks().getSize();
}
