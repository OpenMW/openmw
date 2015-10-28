#include "startscriptcreator.hpp"

CSVWorld::StartScriptCreator::StartScriptCreator(CSMWorld::Data &data, QUndoStack &undoStack, const CSMWorld::UniversalId &id, bool relaxedIdRules):
    GenericCreator (data, undoStack, id, true)
{}

std::string CSVWorld::StartScriptCreator::getErrors() const
{
    std::string errors;

    errors = getIdValidatorResult();
    if (errors.length() > 0)
        return errors;
    else if (getData().getScripts().searchId(getId()) == -1)
        errors = "Script ID not found";
    else if (getData().getStartScripts().searchId(getId()) > -1 )
        errors = "Script with this ID already registered as Start Script";

    return errors;
}
