#include "pathgridcreator.hpp"

#include <QComboBox>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QStringListModel>

#include "../../model/world/data.hpp"

std::string CSVWorld::PathgridCreator::getId() const
{
    return mCell->currentText().toUtf8().constData();
}

CSVWorld::PathgridCreator::PathgridCreator(
    CSMWorld::Data& data,
    QUndoStack& undoStack,
    const CSMWorld::UniversalId& id,
    bool relaxedIdRules
) : GenericCreator(data, undoStack, id, relaxedIdRules)
{
    setManualEditing(false);

    QLabel *label = new QLabel("Cell ID", this);
    insertBeforeButtons(label, false);

    // Create combo box with case-insensitive sorting.
    mCell = new QComboBox(this);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel;
    QStringListModel *listModel = new QStringListModel;
    proxyModel->setSourceModel(listModel);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    mCell->setModel(proxyModel);
    insertBeforeButtons(mCell, true);

    // Populate combo box with cells that don't have a pathgrid yet.
    const CSMWorld::IdCollection<CSMWorld::Pathgrid>& pathgrids = getData().getPathgrids();
    const CSMWorld::IdCollection<CSMWorld::Cell>& cells = getData().getCells();
    const int cellCount = cells.getSize();
    for (int i = 0; i < cellCount; ++i)
    {
        std::string cellId = cells.getId(i);
        if (pathgrids.searchId(cellId) == -1)
        {
            mCell->addItem(QString::fromStdString(cellId));
        }
    }

    mCell->model()->sort(0);
    mCell->setCurrentIndex(0);

    connect(mCell, SIGNAL (currentIndexChanged(const QString&)), this, SLOT (cellChanged()));
}

std::string CSVWorld::PathgridCreator::getErrors() const
{
    std::string cellId = getId();

    // Check user input for any errors.
    // The last two checks, cell with existing pathgrid and non-existent cell,
    // shouldn't be needed but we absolutely want to make sure they never happen.
    std::string errors;
    if (cellId.empty())
    {
        errors = "No cell ID selected";
    }
    else if (getData().getPathgrids().searchId(cellId) > -1)
    {
        errors = "Pathgrid for selected cell ID already exists";
    }
    else if (getData().getCells().searchId(cellId) == -1)
    {
        errors = "Cell with selected cell ID does not exist";
    }

    return errors;
}

void CSVWorld::PathgridCreator::focus()
{
    mCell->setFocus();
}

void CSVWorld::PathgridCreator::reset()
{
    CSVWorld::GenericCreator::reset();
    mCell->setCurrentIndex(0);
}

void CSVWorld::PathgridCreator::cellChanged()
{
    update();
}
