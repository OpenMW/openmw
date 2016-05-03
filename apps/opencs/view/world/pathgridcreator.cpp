#include "pathgridcreator.hpp"

#include "../../model/world/data.hpp"

CSVWorld::PathgridCreator::PathgridCreator(
    CSMWorld::Data& data,
    QUndoStack& undoStack,
    const CSMWorld::UniversalId& id,
    bool relaxedIdRules
) : GenericCreator(data, undoStack, id, relaxedIdRules)
{}

std::string CSVWorld::PathgridCreator::getErrors() const
{
    std::string pathgridId = getId();

    // Check user input for any errors.
    std::string errors;
    if (pathgridId.empty())
    {
        errors = "No Pathgrid ID entered";
    }
    else if (getData().getPathgrids().searchId(pathgridId) > -1)
    {
        errors = "Pathgrid with this ID already exists";
    }

    return errors;
}
