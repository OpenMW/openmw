#include "referenceablecreator.hpp"

#include <QComboBox>
#include <QLabel>

#include "../../model/world/universalid.hpp"
#include "../../model/world/commands.hpp"

void CSVWorld::ReferenceableCreator::configureCreateCommand (CSMWorld::CreateCommand& command) const
{
    command.setType (
        static_cast<CSMWorld::UniversalId::Type> (mType->itemData (mType->currentIndex()).toInt()));
}

CSVWorld::ReferenceableCreator::ReferenceableCreator (CSMWorld::Data& data, QUndoStack& undoStack,
    const CSMWorld::UniversalId& id)
: GenericCreator (data, undoStack, id)
{
    QLabel *label = new QLabel ("Type", this);
    insertBeforeButtons (label, false);

    std::vector<CSMWorld::UniversalId::Type> types = CSMWorld::UniversalId::listReferenceableTypes();

    mType = new QComboBox (this);

    for (std::vector<CSMWorld::UniversalId::Type>::const_iterator iter (types.begin());
         iter!=types.end(); ++iter)
    {
        CSMWorld::UniversalId id2 (*iter, "");

        mType->addItem (QIcon (id2.getIcon().c_str()), id2.getTypeName().c_str(),
            static_cast<int> (id2.getType()));
    }

    insertBeforeButtons (mType, false);
}

void CSVWorld::ReferenceableCreator::reset()
{
    mType->setCurrentIndex (0);
    GenericCreator::reset();
}

void CSVWorld::ReferenceableCreator::cloneMode (const std::string& originId,
    const CSMWorld::UniversalId::Type type)
{
    GenericCreator::cloneMode (originId, type);
    mType->setCurrentIndex (mType->findData (static_cast<int> (type)));
}

void CSVWorld::ReferenceableCreator::toggleWidgets(bool active)
{
    CSVWorld::GenericCreator::toggleWidgets(active);
    mType->setEnabled(active);
}
