
#include "referencecreator.hpp"

#include <QLabel>
#include <QLineEdit>

#include "../../model/world/data.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/idtable.hpp"

std::string CSVWorld::ReferenceCreator::getId() const
{
    return mId;
}

void CSVWorld::ReferenceCreator::configureCreateCommand (CSMWorld::CreateCommand& command) const
{
    int index =
        dynamic_cast<CSMWorld::IdTable&> (*getData().getTableModel (getCollectionId())).
        findColumnIndex (CSMWorld::Columns::ColumnId_Cell);

    command.addValue (index, mCell->text());
}

void CSVWorld::ReferenceCreator::pushCommand (std::auto_ptr<QUndoCommand> command,
    const std::string& id)
{
    // get the old count
    std::string cellId = mCell->text().toUtf8().constData();

    CSMWorld::IdTable& cellTable = dynamic_cast<CSMWorld::IdTable&> (
        *getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

    int countColumn = cellTable.findColumnIndex (CSMWorld::Columns::ColumnId_RefNumCounter);

    QModelIndex countIndex = cellTable.getModelIndex (cellId, countColumn);

    int count = cellTable.data (countIndex).toInt();

    // command for setting the refnum in the newly created reference
    CSMWorld::IdTable& referenceTable = dynamic_cast<CSMWorld::IdTable&> (
        *getData().getTableModel (CSMWorld::UniversalId::Type_References));

    int refNumColumn = referenceTable.findColumnIndex (CSMWorld::Columns::ColumnId_RefNum);

    std::auto_ptr<CSMWorld::ModifyCommand> setRefNum (new CSMWorld::ModifyCommand
        (referenceTable, referenceTable.getModelIndex (id, refNumColumn), count));

    // command for incrementing counter
    std::auto_ptr<CSMWorld::ModifyCommand> increment (new CSMWorld::ModifyCommand
        (cellTable, countIndex, count+1));

    getUndoStack().beginMacro (command->text());
    GenericCreator::pushCommand (command, id);
    getUndoStack().push (setRefNum.release());
    getUndoStack().push (increment.release());
    getUndoStack().endMacro();
}

CSVWorld::ReferenceCreator::ReferenceCreator (CSMWorld::Data& data, QUndoStack& undoStack,
    const CSMWorld::UniversalId& id)
: GenericCreator (data, undoStack, id)
{
    QLabel *label = new QLabel ("Cell", this);
    insertBeforeButtons (label, false);

    mCell = new QLineEdit (this);
    insertBeforeButtons (mCell, true);

    setManualEditing (false);

    connect (mCell, SIGNAL (textChanged (const QString&)), this, SLOT (cellChanged()));
}

void CSVWorld::ReferenceCreator::reset()
{
    GenericCreator::reset();
    mCell->setText ("");
    mId = getData().getReferences().getNewId();
}

std::string CSVWorld::ReferenceCreator::getErrors() const
{
    // We are ignoring errors coming from GenericCreator here, because the ID of the new
    // record is internal and requires neither user input nor verification.
    std::string errors;

    if (mCloneMode)
    {
        return errors;
    }

    std::string cell = mCell->text().toUtf8().constData();

    if (cell.empty())
    {
        if (!errors.empty())
            errors += "<br>";

        errors += "Missing Cell ID";
    }
    else if (getData().getCells().searchId (cell)==-1)
    {
        if (!errors.empty())
            errors += "<br>";

        errors += "Invalid Cell ID";
    }

    return errors;
}

void CSVWorld::ReferenceCreator::cellChanged()
{
    update();
}

void CSVWorld::ReferenceCreator::toggleWidgets(bool active)
{
    CSVWorld::GenericCreator::toggleWidgets(active);
    mCell->setEnabled(active);
}

void CSVWorld::ReferenceCreator::cloneMode(const std::string& originId,
                                           const CSMWorld::UniversalId::Type type)
{
    CSVWorld::GenericCreator::cloneMode(originId, type);
    cellChanged(); //otherwise ok button will remain disabled
}
