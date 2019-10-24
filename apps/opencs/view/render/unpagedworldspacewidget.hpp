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

            virtual dropRequirments getDropRequirements(DropType type) const;

            /// \return Drop handled?
            virtual bool handleDrop (const std::vector<CSMWorld::UniversalId>& data,
                DropType type);

            /// \param elementMask Elements to be affected by the clear operation
            virtual void clearSelection (int elementMask);

            /// \param elementMask Elements to be affected by the select operation
            virtual void invertSelection (int elementMask);

            /// \param elementMask Elements to be affected by the select operation
            virtual void selectAll (int elementMask);

            // Select everything that references the same ID as at least one of the elements
            // already selected
            //
            /// \param elementMask Elements to be affected by the select operation
            virtual void selectAllWithSameParentId (int elementMask);

            virtual std::string getCellId (const osg::Vec3f& point) const;

            virtual Cell* getCell(const osg::Vec3d& point) const;

            virtual Cell* getCell(const CSMWorld::CellCoordinates& coords) const;

            virtual std::vector<osg::ref_ptr<TagBase> > getSelection (unsigned int elementMask)
                const;

            virtual std::vector<osg::ref_ptr<TagBase> > getEdited (unsigned int elementMask)
                const;

            virtual void setSubMode (int subMode, unsigned int elementMask);

            /// Erase all overrides and restore the visual representation to its true state.
            virtual void reset (unsigned int elementMask);

        private:

            virtual void referenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight);

            virtual void referenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            virtual void referenceableAdded (const QModelIndex& index, int start, int end);

            virtual void referenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            virtual void referenceAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            virtual void referenceAdded (const QModelIndex& index, int start, int end);

            virtual void pathgridDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            virtual void pathgridAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            virtual void pathgridAdded (const QModelIndex& parent, int start, int end);


            virtual std::string getStartupInstruction();

        protected:

            virtual void addVisibilitySelectorButtons (CSVWidget::SceneToolToggle2 *tool);

        private slots:

            void cellDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void cellRowsAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            void assetTablesChanged ();

        signals:

            void cellChanged(const CSMWorld::UniversalId& id);
    };
}

#endif
