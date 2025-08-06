#include "referenceablecreator.hpp"

#include <vector>

#include <QComboBox>
#include <QLabel>

#include <apps/opencs/view/world/genericcreator.hpp>

#include <components/misc/scalableicon.hpp>

#include "../../model/world/commands.hpp"
#include "../../model/world/universalid.hpp"

class QUndoStack;

namespace CSMWorld
{
    class Data;
}

void CSVWorld::ReferenceableCreator::configureCreateCommand(CSMWorld::CreateCommand& command) const
{
    command.setType(static_cast<CSMWorld::UniversalId::Type>(mType->itemData(mType->currentIndex()).toInt()));
}

CSVWorld::ReferenceableCreator::ReferenceableCreator(
    CSMWorld::Data& worldData, QUndoStack& undoStack, const CSMWorld::UniversalId& id)
    : GenericCreator(worldData, undoStack, id)
{
    QLabel* label = new QLabel("Type", this);
    insertBeforeButtons(label, false);

    std::vector<CSMWorld::UniversalId::Type> types = CSMWorld::UniversalId::listReferenceableTypes();

    mType = new QComboBox(this);
    mType->setMaxVisibleItems(20);

    for (std::vector<CSMWorld::UniversalId::Type>::const_iterator iter(types.begin()); iter != types.end(); ++iter)
    {
        CSMWorld::UniversalId id2(*iter, "");

        mType->addItem(Misc::ScalableIcon::load(id2.getIcon().c_str()), id2.getTypeName().c_str(),
            static_cast<int>(id2.getType()));
    }

    mType->model()->sort(0);

    insertBeforeButtons(mType, false);

    connect(mType, qOverload<int>(&QComboBox::currentIndexChanged), this, &ReferenceableCreator::setType);
}

void CSVWorld::ReferenceableCreator::reset()
{
    mType->setCurrentIndex(0);
    GenericCreator::reset();
}

void CSVWorld::ReferenceableCreator::setType(int index)
{
    // container items have name limit of 32 characters
    std::string text = mType->currentText().toStdString();
    if (text == "Potion" || text == "Apparatus" || text == "Armor" || text == "Book" || text == "Clothing"
        || text == "Ingredient" || text == "ItemLevelledList" || text == "Light" || text == "Lockpick"
        || text == "Miscellaneous" || text == "Probe" || text == "Repair" || text == "Weapon")
    {
        GenericCreator::setEditorMaxLength(32);
    }
    else
        GenericCreator::setEditorMaxLength(32767);
}

void CSVWorld::ReferenceableCreator::cloneMode(const std::string& originId, const CSMWorld::UniversalId::Type type)
{
    GenericCreator::cloneMode(originId, type);
    mType->setCurrentIndex(mType->findData(static_cast<int>(type)));
}

void CSVWorld::ReferenceableCreator::toggleWidgets(bool active)
{
    CSVWorld::GenericCreator::toggleWidgets(active);
    mType->setEnabled(active);
}
