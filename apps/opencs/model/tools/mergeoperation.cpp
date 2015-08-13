
#include "mergeoperation.hpp"

#include "../doc/state.hpp"

CSMTools::MergeOperation::MergeOperation (CSMDoc::Document& document)
: CSMDoc::Operation (CSMDoc::State_Merging, true)
{

}

void CSMTools::MergeOperation::setTarget (const boost::filesystem::path& target)
{

}
