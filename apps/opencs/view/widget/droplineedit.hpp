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
            DropLineEdit(CSMWorld::ColumnBase::Display type, QWidget *parent = nullptr);

        protected:
            void dragEnterEvent(QDragEnterEvent *event) override;
            void dragMoveEvent(QDragMoveEvent *event) override;
            void dropEvent(QDropEvent *event) override;

        signals:
            void tableMimeDataDropped(const CSMWorld::UniversalId &id, const CSMDoc::Document *document);
    };
}

#endif
