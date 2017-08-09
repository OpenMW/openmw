#ifndef CSV_WORLD_REGIONMAP_H
#define CSV_WORLD_REGIONMAP_H

#include <cstddef>
#include <vector>

#include <QObject>
#include <QTableView>

#include "./dragrecordtable.hpp"

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
    class RegionMap : public DragRecordTable
    {
            Q_OBJECT

            QAction *mSelectAllAction;
            QAction *mClearSelectionAction;
            QAction *mSelectRegionsAction;
            QAction *mCreateCellsAction;
            QAction *mSetRegionAction;
            QAction *mUnsetRegionAction;
            QAction *mViewAction;
            QAction *mViewInTableAction;
            std::string mRegionId;

        private:

            void contextMenuEvent (QContextMenuEvent *event);

            QModelIndexList getUnselectedCells() const;
            ///< \note Non-existent cells are not listed.

            QModelIndexList getSelectedCells (bool existent = true, bool nonExistent = false) const;
            ///< \param existent Include existent cells.
            /// \param nonExistent Include non-existent cells.

            QModelIndexList getMissingRegionCells() const;
            ///< Unselected cells within all regions that have at least one selected cell.

            void setRegion (const std::string& regionId);
            ///< Set region Id of selected cells.

            void mouseMoveEvent(QMouseEvent *event);

            void dropEvent(QDropEvent* event);

        public:

            RegionMap (const CSMWorld::UniversalId& universalId, CSMDoc::Document& document,
                QWidget *parent = 0);

            virtual std::vector<CSMWorld::UniversalId> getDraggedRecords() const;

        signals:

            void editRequest (const CSMWorld::UniversalId& id, const std::string& hint);

        private slots:

            void selectAll();

            void clearSelection();

            void selectRegions();

            void createCells();

            void setRegion();

            void unsetRegion();

            void view();

            void viewInTable();
    };
}

#endif
