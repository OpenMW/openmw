#ifndef CSV_WORLD_REGIONMAP_H
#define CSV_WORLD_REGIONMAP_H

#include <QTableView>

class QAction;
class QAbstractItemModel;

namespace CSVWorld
{
    class RegionMap : public QTableView
    {
            Q_OBJECT

            QAction *mSelectAllAction;
            QAction *mClearSelectionAction;
            QAction *mSelectRegionsAction;

        private:

            void contextMenuEvent (QContextMenuEvent *event);

            QModelIndexList getUnselectedCells() const;
            ///< Note non-existent cells are not listed.

            QModelIndexList getMissingRegionCells() const;
            ///< Unselected cells within all regions that have at least one selected cell.

        public:

            RegionMap (QAbstractItemModel *model, QWidget *parent = 0);

            void setEditLock (bool locked);

        private slots:

            void selectAll();

            void clearSelection();

            void selectRegions();
    };
}

#endif
