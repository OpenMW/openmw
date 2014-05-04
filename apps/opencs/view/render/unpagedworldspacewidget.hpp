#ifndef OPENCS_VIEW_UNPAGEDWORLDSPACEWIDGET_H
#define OPENCS_VIEW_UNPAGEDWORLDSPACEWIDGET_H

#include <string>

#include "worldspacewidget.hpp"

class QModelIndex;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class IdTable;
}

namespace CSVRender
{
    class UnpagedWorldspaceWidget : public WorldspaceWidget
    {
            Q_OBJECT

            std::string mCellId;
            CSMWorld::IdTable *mCellsModel;

            void update();

        public:

            UnpagedWorldspaceWidget (const std::string& cellId, CSMDoc::Document& document,
                QWidget *parent);

        private slots:

            void cellDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void cellRowsAboutToBeRemoved (const QModelIndex& parent, int start, int end);
    };
}

#endif
