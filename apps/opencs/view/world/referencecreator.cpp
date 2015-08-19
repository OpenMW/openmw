#include "referencecreator.hpp"

#include <QLabel>

#include "../../model/doc/document.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/idcompletionmanager.hpp"

#include "../widget/droplineedit.hpp"

std::string CSVWorld::ReferenceCreator::getId() const
{
    return mId;
}

void CSVWorld::ReferenceCreator::configureCreateCommand (CSMWorld::CreateCommand& command) const
{
    // Set cellID
    int cellIdColumn =
        dynamic_cast<CSMWorld::IdTable&> (*getData().getTableModel (getCollectionId())).
        findColumnIndex (CSMWorld::Columns::ColumnId_Cell);

    command.addValue (cellIdColumn, mCell->text());

    // Set RefNum
    int refNumColumn = dynamic_cast<CSMWorld::IdTable&> (
        *getData().getTableModel (CSMWorld::UniversalId::Type_References)).
        findColumnIndex (CSMWorld::Columns::ColumnId_RefNum);

    command.addValue (refNumColumn, getRefNumCount());
}

void CSVWorld::ReferenceCreator::pushCommand (std::auto_ptr<CSMWorld::CreateCommand> command,
    const std::string& id)
{
    // get the old count
    std::string cellId = mCell->text().toUtf8().constData();

    CSMWorld::IdTable& cellTable = dynamic_cast<CSMWorld::IdTable&> (
        *getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

    int countColumn = cellTable.findColumnIndex (CSMWorld::Columns::ColumnId_RefNumCounter);

    QModelIndex countIndex = cellTable.getModelIndex (cellId, countColumn);

    int count = cellTable.data (countIndex).toInt();

    // command for incrementing counter
    std::auto_ptr<CSMWorld::ModifyCommand> increment (new CSMWorld::ModifyCommand
        (cellTable, countIndex, count+1));

    getUndoStack().beginMacro (command->text());
    GenericCreator::pushCommand (command, id);
    getUndoStack().push (increment.release());
    getUndoStack().endMacro();
}

int CSVWorld::ReferenceCreator::getRefNumCount() const
{
    std::string cellId = mCell->text().toUtf8().constData();

    CSMWorld::IdTable& cellTable = dynamic_cast<CSMWorld::IdTable&> (
        *getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

    int countColumn = cellTable.findColumnIndex (CSMWorld::Columns::ColumnId_RefNumCounter);

    QModelIndex countIndex = cellTable.getModelIndex (cellId, countColumn);

    return cellTable.data (countIndex).toInt();
}

CSVWorld::ReferenceCreator::ReferenceCreator (CSMWorld::Data& data, QUndoStack& undoStack,
    const CSMWorld::UniversalId& id, CSMWorld::IdCompletionManager &completionManager)
: GenericCreator (data, undoStack, id)
{
    QLabel *label = new QLabel ("Cell", this);
    insertBeforeButtons (label, false);

    mCell = new CSVWidget::DropLineEdit(CSMWorld::ColumnBase::Display_Cell, this);
    mCell->setCompleter(completionManager.getCompleter(CSMWorld::ColumnBase::Display_Cell).get());
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

void CSVWorld::ReferenceCreator::focus()
{
    mCell->setFocus();
}

void CSVWorld::ReferenceCreator::cellChanged()
{
    update();
}

void CSVWorld::ReferenceCreator::cloneMode(const std::string& originId,
                                           const CSMWorld::UniversalId::Type type)
{
    CSMWorld::IdTable& referenceTable = dynamic_cast<CSMWorld::IdTable&> (
        *getData().getTableModel (CSMWorld::UniversalId::Type_References));

    int cellIdColumn = referenceTable.findColumnIndex (CSMWorld::Columns::ColumnId_Cell);

    mCell->setText (
        referenceTable.data (referenceTable.getModelIndex (originId, cellIdColumn)).toString());

    CSVWorld::GenericCreator::cloneMode(originId, type);
    cellChanged(); //otherwise ok button will remain disabled
}

CSVWorld::Creator *CSVWorld::ReferenceCreatorFactory::makeCreator (CSMDoc::Document& document, 
                                                                   const CSMWorld::UniversalId& id) const
{
    return new ReferenceCreator(document.getData(), 
                                document.getUndoStack(), 
                                id,
                                document.getIdCompletionManager());
}
