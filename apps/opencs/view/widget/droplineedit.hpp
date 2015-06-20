#ifndef CSV_WIDGET_DROPLINEEDIT_HPP
#define CSV_WIDGET_DROPLINEEDIT_HPP

#include <QLineEdit>

#include "../../model/world/columnbase.hpp"
#include "../../model/world/universalid.hpp"

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class TableMimeData;
}

namespace CSVWidget
{
    class DropLineEdit : public QLineEdit
    {
            Q_OBJECT

            CSMWorld::UniversalId::Type mDropType;
            ///< The accepted ID type for this LineEdit.
            ///< If \a mDropType has Type_None type, this LineEdit accepts all ID types

            bool canAcceptEventData(QDropEvent *event) const;
            ///< Checks whether the \a event contains CSMWorld::TableMimeData with a proper ID type

            int getAcceptedDataIndex(const CSMWorld::TableMimeData &data) const;
            ///< Checks whether the \a data has a proper type
            ///< \return -1 if there is no suitable data (ID type)

        public:
            DropLineEdit(CSMWorld::UniversalId::Type type, QWidget *parent = 0);
            DropLineEdit(CSMWorld::ColumnBase::Display display, QWidget *parent = 0);

        protected:
            void dragEnterEvent(QDragEnterEvent *event);
            void dragMoveEvent(QDragMoveEvent *event);
            void dropEvent(QDropEvent *event);

        signals:
            void tableMimeDataDropped(const CSMWorld::UniversalId &id, const CSMDoc::Document *document);
    };
}

#endif
