#ifndef OPENCS_VIEW_UNPAGEDWORLDSPACEWIDGET_H
#define OPENCS_VIEW_UNPAGEDWORLDSPACEWIDGET_H

#include <string>
#include <memory>

#include "worldspacewidget.hpp"
#include "cell.hpp"

class QModelIndex;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class IdTable;
    class CellCoordinates;
}

namespace CSVRender
{
    class UnpagedWorldspaceWidget : public WorldspaceWidget
    {
            Q_OBJECT

            CSMDoc::Document& mDocument;
            std::string mCellId;
            CSMWorld::IdTable *mCellsModel;
            CSMWorld::IdTable *mReferenceablesModel;
            std::unique_ptr<Cell> mCell;

            void update();

        public:

            UnpagedWorldspaceWidget (const std::string& cellId, CSMDoc::Document& document,
                                     QWidget *parent);

            dropRequirments getDropRequirements(DropType type) const override;

            /// \return Drop handled?
            bool handleDrop (const std::vector<CSMWorld::UniversalId>& data,
                DropType type) override;

            /// \param elementMask Elements to be affected by the clear operation
            void clearSelection (int elementMask) override;

            /// \param elementMask Elements to be affected by the select operation
            void invertSelection (int elementMask) override;

            /// \param elementMask Elements to be affected by the select operation
            void selectAll (int elementMask) override;

            // Select everything that references the same ID as at least one of the elements
            // already selected
            //
            /// \param elementMask Elements to be affected by the select operation
            void selectAllWithSameParentId (int elementMask) override;

            void selectInsideCube(const osg::Vec3d& pointA, const osg::Vec3d& pointB, DragMode dragMode) override;

            void selectWithinDistance(const osg::Vec3d& point, float distance, DragMode dragMode) override;

            std::string getCellId (const osg::Vec3f& point) const override;

            Cell* getCell(const osg::Vec3d& point) const override;

            Cell* getCell(const CSMWorld::CellCoordinates& coords) const override;

            std::vector<osg::ref_ptr<TagBase> > getSelection (unsigned int elementMask)
                const override;

            std::vector<osg::ref_ptr<TagBase> > getEdited (unsigned int elementMask)
                const override;

            void setSubMode (int subMode, unsigned int elementMask) override;

            /// Erase all overrides and restore the visual representation to its true state.
            void reset (unsigned int elementMask) override;

        private:

            void referenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight) override;

            void referenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end) override;

            void referenceableAdded (const QModelIndex& index, int start, int end) override;

            void referenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight) override;

            void referenceAboutToBeRemoved (const QModelIndex& parent, int start, int end) override;

            void referenceAdded (const QModelIndex& index, int start, int end) override;

            void pathgridDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight) override;

            void pathgridAboutToBeRemoved (const QModelIndex& parent, int start, int end) override;

            void pathgridAdded (const QModelIndex& parent, int start, int end) override;

            std::string getStartupInstruction()  override;

        protected:

            void addVisibilitySelectorButtons (CSVWidget::SceneToolToggle2 *tool) override;

        private slots:

            void cellDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void cellRowsAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            void assetTablesChanged ();

        signals:

            void cellChanged(const CSMWorld::UniversalId& id);
    };
}

#endif
