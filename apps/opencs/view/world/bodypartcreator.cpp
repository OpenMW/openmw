#include "bodypartcreator.hpp"

#include <QCheckBox>

#include "../../model/world/data.hpp"
#include "../../model/world/universalid.hpp"

std::string CSVWorld::BodyPartCreator::getId() const
{
    std::string id = CSVWorld::GenericCreator::getId();

    if (mFirstPerson->isChecked())
    {
        id += ".1st";
    }

    return id;
}

CSVWorld::BodyPartCreator::BodyPartCreator(
    CSMWorld::Data& data,
    QUndoStack& undoStack,
    const CSMWorld::UniversalId& id
) : GenericCreator(data, undoStack, id)
{
    mFirstPerson = new QCheckBox("First Person", this);
    insertBeforeButtons(mFirstPerson, false);

    connect(mFirstPerson, SIGNAL(clicked(bool)), this, SLOT(checkboxClicked()));
}

std::string CSVWorld::BodyPartCreator::getErrors() const
{
    std::string errors;

    std::string id = getId();
    if (getData().hasId(id))
    {
        errors = "ID is already in use";
    }

    return errors;
}

void CSVWorld::BodyPartCreator::reset()
{
    CSVWorld::GenericCreator::reset();
    mFirstPerson->setChecked(false);
}

void CSVWorld::BodyPartCreator::checkboxClicked()
{
    update();
}
