#ifndef CSV_WIDGET_DROPLINEEDIT_HPP
#define CSV_WIDGET_DROPLINEEDIT_HPP

#include <QLineEdit>

#include "../../model/world/columnbase.hpp"

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class TableMimeData;
    class UniversalId;
}

namespace CSVWidget
{
    class DropLineEdit : public QLineEdit
    {
            Q_OBJECT

            CSMWorld::ColumnBase::Display mDropType;
            ///< The accepted Display type for this LineEdit.

        public:
            DropLineEdit(CSMWorld::ColumnBase::Display type, QWidget *parent = 0);

        protected:
            void dragEnterEvent(QDragEnterEvent *event);
            void dragMoveEvent(QDragMoveEvent *event);
            void dropEvent(QDropEvent *event);

        signals:
            void tableMimeDataDropped(const CSMWorld::UniversalId &id, const CSMDoc::Document *document);
    };
}

#endif
