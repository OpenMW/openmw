#include "referencecreator.hpp"

#include <QLabel>

#include "../../model/doc/document.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/idcompletionmanager.hpp"
#include "../../model/world/commandmacro.hpp"

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
}

CSVWorld::ReferenceCreator::ReferenceCreator (CSMWorld::Data& data, QUndoStack& undoStack,
    const CSMWorld::UniversalId& id, CSMWorld::IdCompletionManager &completionManager)
: GenericCreator (data, undoStack, id)
{
    QLabel *label = new QLabel ("Cell", this);
    insertBeforeButtons (label, false);

    // Add cell ID input with auto-completion.
    // Only existing cell IDs are accepted so no ID validation is performed.
    mCell = new CSVWidget::DropLineEdit(CSMWorld::ColumnBase::Display_Cell, this);
    mCell->setCompleter(completionManager.getCompleter(CSMWorld::ColumnBase::Display_Cell).get());
    insertBeforeButtons (mCell, true);

    setManualEditing (false);

    connect (mCell, SIGNAL (textChanged (const QString&)), this, SLOT (cellChanged()));
    connect (mCell, SIGNAL (returnPressed()), this, SLOT (inputReturnPressed()));
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
        errors += "Missing Cell ID";
    else if (getData().getCells().searchId (cell)==-1)
        errors += "Invalid Cell ID";

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
