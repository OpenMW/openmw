
#include "dialoguesubview.hpp"

#include <QGridLayout>
#include <QLabel>
#include <QAbstractTableModel>

#include "../../model/world/columnbase.hpp"

CSVWorld::DialogueSubView::DialogueSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document,
    bool createAndDelete)
: SubView (id)
{
    QWidget *widget = new QWidget (this);

    setWidget (widget);

    QGridLayout *layout = new QGridLayout;

    widget->setLayout (layout);

    QAbstractTableModel *model = document.getData().getTableModel (id);

    int columns = model->columnCount();

    for (int i=0; i<columns; ++i)
    {
        int flags = model->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Flags).toInt();

        if (flags & CSMWorld::ColumnBase::Flag_Dialogue)
        {
            layout->addWidget (new QLabel (model->headerData (i, Qt::Horizontal).toString()), i, 0);
        }
    }
}

void CSVWorld::DialogueSubView::setEditLock (bool locked)
{

}