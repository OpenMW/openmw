#ifndef CSV_RENDER_TERRAINSHAPEMODE_H
#define CSV_RENDER_TERRAINSHAPEMODE_H

#include "editmode.hpp"

#include <string>
#include <memory>

#include <QWidget>
#include <QEvent>

#ifndef Q_MOC_RUN
#include "../../model/world/data.hpp"
#include "../../model/world/land.hpp"

#include "../../model/doc/document.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/landtexture.hpp"
#endif

#include "terrainselection.hpp"

namespace CSVWidget
{
    class SceneToolShapeBrush;
}

namespace CSVRender
{
    class PagedWorldspaceWidget;

    /// \brief EditMode for handling the terrain shape editing
    class TerrainShapeMode : public EditMode
    {
        Q_OBJECT

        public:

            enum InteractionType
            {
                InteractionType_PrimaryEdit,
                InteractionType_PrimarySelect,
                InteractionType_SecondaryEdit,
                InteractionType_SecondarySelect,
                InteractionType_None
            };

            /// Editmode for terrain shape grid
            TerrainShapeMode(WorldspaceWidget*, osg::Group* parentNode, QWidget* parent = nullptr);

            void primaryOpenPressed (const WorldspaceHitResult& hit);

            /// Create single command for one-click shape editing
            void primaryEditPressed (const WorldspaceHitResult& hit);

            /// Open brush settings window
            void primarySelectPressed(const WorldspaceHitResult&);

            void secondarySelectPressed(const WorldspaceHitResult&);

            void activate(CSVWidget::SceneToolbar*);
            void deactivate(CSVWidget::SceneToolbar*);

            /// Start shape editing command macro
            virtual bool primaryEditStartDrag (const QPoint& pos);

            virtual bool secondaryEditStartDrag (const QPoint& pos);
            virtual bool primarySelectStartDrag (const QPoint& pos);
            virtual bool secondarySelectStartDrag (const QPoint& pos);

            /// Handle shape edit behavior during dragging
            virtual void drag (const QPoint& pos, int diffX, int diffY, double speedFactor);

            /// End shape editing command macro
            virtual void dragCompleted(const QPoint& pos);

            /// Cancel shape editing, and reset all pending changes
            virtual void dragAborted();

            virtual void dragWheel (int diff, double speedFactor);
            virtual void dragMoveEvent (QDragMoveEvent *event);

            /// Handle brush mechanics for shape editing
            void editTerrainShapeGrid (const std::pair<int, int>& vertexCoords, bool dragOperation);

            /// Do a single height alteration for transient shape edit map
            void alterHeight(const CSMWorld::CellCoordinates& cellCoords, int inCellX, int inCellY, float alteredHeight, bool useTool = true);

            /// Do a single smoothing height alteration for transient shape edit map
            void smoothHeight(const CSMWorld::CellCoordinates& cellCoords, int inCellX, int inCellY, int toolStrength);

            /// Do a single flattening height alteration for transient shape edit map
            void flattenHeight(const CSMWorld::CellCoordinates& cellCoords, int inCellX, int inCellY, int toolStrength, int targetHeight);

            /// Get altered height values around one vertex
            void updateKeyHeightValues(const CSMWorld::CellCoordinates& cellCoords, int inCellX, int inCellY, float* thisHeight,
                float* thisAlteredHeight, float* leftHeight, float* leftAlteredHeight, float* upHeight, float* upAlteredHeight,
                float* rightHeight, float* rightAlteredHeight, float* downHeight, float* downAlteredHeight);

            ///Limit steepness based on either X or Y and return false if steepness is limited
            void compareAndLimit(const CSMWorld::CellCoordinates& cellCoords, int inCellX, int inCellY, float* limitedAlteredHeightXAxis,
                float* limitedAlteredHeightYAxis, bool* steepnessIsWithinLimits);

            /// Check that the edit doesn't break save format limits, fix if necessary, return true if slope steepness is within limits
            bool limitAlteredHeights(const CSMWorld::CellCoordinates& cellCoords, bool reverseMode = false);

            /// Handle brush mechanics for terrain shape selection
            void selectTerrainShapes (const std::pair<int, int>& vertexCoords, unsigned char selectMode, bool dragOperation);

            /// Push terrain shape edits to command macro
            void pushEditToCommand (const CSMWorld::LandHeightsColumn::DataType& newLandGrid, CSMDoc::Document& document,
                CSMWorld::IdTable& landTable, std::string cellId);

            /// Push land normals edits to command macro
            void pushNormalsEditToCommand(const CSMWorld::LandNormalsColumn::DataType& newLandGrid, CSMDoc::Document& document,
                CSMWorld::IdTable& landTable, std::string cellId);

            /// Create new cell and land if needed
            bool allowLandShapeEditing(std::string textureFileName);

        private:
            std::string mCellId;
            std::string mBrushTexture;
            int mBrushSize;
            int mBrushShape;
            std::vector<std::pair<int, int>> mCustomBrushShape;
            CSVWidget::SceneToolShapeBrush *mShapeBrushScenetool;
            int mDragMode;
            osg::Group* mParentNode;
            bool mIsEditing;
            std::unique_ptr<TerrainSelection> mTerrainShapeSelection;
            int mTotalDiffY;
            std::vector<CSMWorld::CellCoordinates> mAlteredCells;
            osg::Vec3d mEditingPos;
            int mShapeEditTool;
            int mShapeEditToolStrength;
            int mTargetHeight;

            const int cellSize {ESM::Land::REAL_SIZE};
            const int landSize {ESM::Land::LAND_SIZE};
            const int landTextureSize {ESM::Land::LAND_TEXTURE_SIZE};

            PagedWorldspaceWidget& getPagedWorldspaceWidget();

        signals:
            void passBrushTexture(std::string brushTexture);

        public slots:
            void setBrushSize(int brushSize);
            void setBrushShape(int brushShape);
            void setShapeEditTool(int shapeEditTool);
            void setShapeEditToolStrength(int shapeEditToolStrength);
    };
}


#endif
