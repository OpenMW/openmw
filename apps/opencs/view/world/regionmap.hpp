#ifndef CSV_WORLD_REGIONMAP_H
#define CSV_WORLD_REGIONMAP_H

#include <QTableView>

class QAction;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class UniversalId;
}

namespace CSVWorld
{
    class RegionMap : public QTableView
    {
            Q_OBJECT

            QAction *mSelectAllAction;
            QAction *mClearSelectionAction;
            QAction *mSelectRegionsAction;
            QAction *mCreateCellsAction;
            bool mEditLock;
            CSMDoc::Document& mDocument;

        private:

            void contextMenuEvent (QContextMenuEvent *event);

            QModelIndexList getUnselectedCells() const;
            ///< \note Non-existent cells are not listed.

            QModelIndexList getSelectedCells (bool existent = true, bool nonExistent = false) const;
            ///< \param existant Include existant cells.
            /// \param nonExistant Include non-existant cells.

            QModelIndexList getMissingRegionCells() const;
            ///< Unselected cells within all regions that have at least one selected cell.

        public:

            RegionMap (const CSMWorld::UniversalId& universalId, CSMDoc::Document& document,
                QWidget *parent = 0);

            void setEditLock (bool locked);

        private slots:

            void selectAll();

            void clearSelection();

            void selectRegions();

            void createCells();
    };
}

#endif
