
#include "dialoguesubview.hpp"

#include <QGridLayout>
#include <QLabel>
#include <QAbstractItemModel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDataWidgetMapper>

#include "../../model/world/columnbase.hpp"
#include "../../model/world/idtable.hpp"

CSVWorld::DialogueSubView::DialogueSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document,
    bool createAndDelete)
: SubView (id)
{
    QWidget *widget = new QWidget (this);

    setWidget (widget);

    QGridLayout *layout = new QGridLayout;

    widget->setLayout (layout);

    QAbstractItemModel *model = document.getData().getTableModel (id);

    int columns = model->columnCount();

    mWidgetMapper = new QDataWidgetMapper (this);
    mWidgetMapper->setModel (model);

    for (int i=0; i<columns; ++i)
    {
        int flags = model->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Flags).toInt();

        if (flags & CSMWorld::ColumnBase::Flag_Dialogue)
        {
            layout->addWidget (new QLabel (model->headerData (i, Qt::Horizontal).toString()), i, 0);

            CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display>
                (model->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

            QWidget *widget = 0;

            if (model->flags (model->index (0, i)) & Qt::ItemIsEditable)
            {
                switch (display)
                {
                    case CSMWorld::ColumnBase::Display_String:

                        layout->addWidget (widget = new QLineEdit, i, 1);
                        break;

                    case CSMWorld::ColumnBase::Display_Integer:

                        /// \todo configure widget properly (range)
                        layout->addWidget (widget = new QSpinBox, i, 1);
                        break;

                    case CSMWorld::ColumnBase::Display_Float:

                        /// \todo configure widget properly (range, format?)
                        layout->addWidget (widget = new QDoubleSpinBox, i, 1);
                        break;

                    default: break; // silence warnings for other times for now
                }
            }
            else
            {
                switch (display)
                {
                    case CSMWorld::ColumnBase::Display_String:
                    case CSMWorld::ColumnBase::Display_Integer:
                    case CSMWorld::ColumnBase::Display_Float:

                        layout->addWidget (widget = new QLabel, i, 1);
                        break;

                    default: break; // silence warnings for other times for now
                }
            }

            if (widget)
                mWidgetMapper->addMapping (widget, i);
        }
    }

    mWidgetMapper->setCurrentModelIndex (
        dynamic_cast<CSMWorld::IdTable&> (*model).getModelIndex (id.getId(), 0));
}

void CSVWorld::DialogueSubView::setEditLock (bool locked)
{

}