#ifndef CSV_RENDER_TERRAINVERTEXPAINTMODE_H
#define CSV_RENDER_TERRAINVERTEXPAINTMODE_H

#include "editmode.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <apps/opencs/model/world/cellcoordinates.hpp>

#include <osg/Vec3d>

#ifndef Q_MOC_RUN
#include "../../model/world/columnimp.hpp"
#include "../widget/brushshapes.hpp"
#endif

#include "brushdraw.hpp"

class QDragMoveEvent;
class QMouseEvent;
class QObject;
class QPoint;
class QWidget;

namespace osg
{
    class Group;
}

namespace CSMDoc
{
    class Document;
}

namespace CSVWidget
{
    class SceneToolVertexPaintBrush;
}

namespace CSMWorld
{
    class IdTable;
}

namespace CSVRender
{
    class PagedWorldspaceWidget;
    class TerrainSelection;
    class WorldspaceWidget;
    struct WorldspaceHitResult;
    class SceneToolbar;

    /// \brief EditMode for handling the terrain shape editing
    class TerrainVertexPaintMode : public EditMode
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

        enum VertexPaintEditTool
        {
            VertexPaintEditTool_Replace = 0
        };

        /// Editmode for terrain shape grid
        TerrainVertexPaintMode(WorldspaceWidget*, osg::Group* parentNode, QWidget* parent = nullptr);

        void primaryOpenPressed(const WorldspaceHitResult& hit) override;

        /// Create single command for one-click shape editing
        void primaryEditPressed(const WorldspaceHitResult& hit) override;

        /// Open brush settings window
        void primarySelectPressed(const WorldspaceHitResult&) override;

        void secondarySelectPressed(const WorldspaceHitResult&) override;

        void activate(CSVWidget::SceneToolbar*) override;
        void deactivate(CSVWidget::SceneToolbar*) override;

        /// Start shape editing command macro
        bool primaryEditStartDrag(const QPoint& pos) override;

        bool secondaryEditStartDrag(const QPoint& pos) override;
        bool primarySelectStartDrag(const QPoint& pos) override;
        bool secondarySelectStartDrag(const QPoint& pos) override;

        /// Handle shape edit behavior during dragging
        void drag(const QPoint& pos, int diffX, int diffY, double speedFactor) override;

        /// End shape editing command macro
        void dragCompleted(const QPoint& pos) override;

        /// Cancel shape editing, and reset all pending changes
        void dragAborted() override;

        void dragWheel(int diff, double speedFactor) override;
        void dragMoveEvent(QDragMoveEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;

        std::shared_ptr<TerrainSelection> getTerrainSelection();

    private:
        /// Reset everything in the current edit
        void endShapeEditing();

        /// Handle brush mechanics for colour editing
        void editVertexColourGrid(const std::pair<int, int>& vertexCoords, bool dragOperation);

        /// Alter one pixel's colour
        void alterColour(CSMWorld::LandColoursColumn::DataType& landColorsNew, int inCellX, int inCellY,
            float alteredHeight, bool useTool = true);

        /// Check if global selection coordinate belongs to cell in view
        bool isInCellSelection(int globalSelectionX, int globalSelectionY);

        /// Select vertex at global selection coordinate
        void handleSelection(int globalSelectionX, int globalSelectionY, std::vector<std::pair<int, int>>* selections);

        /// Handle brush mechanics for terrain shape selection
        void selectTerrainShapes(const std::pair<int, int>& vertexCoords, unsigned char selectMode);

        bool noCell(const std::string& cellId);

        bool noLand(const std::string& cellId);

        bool noLandLoaded(const std::string& cellId);

        bool isLandLoaded(const std::string& cellId);

        /// Push terrain shape edits to command macro
        void pushEditToCommand(const CSMWorld::LandColoursColumn::DataType& newLandColours, CSMDoc::Document& document,
            CSMWorld::IdTable& landTable, const std::string& cellId);

        /// Create new blank height record and new normals, if there are valid adjancent cell, take sample points and
        /// set the average height based on that
        void createNewLandData(const CSMWorld::CellCoordinates& cellCoords);

        /// Create new cell and land if needed, only user tools may ask for opening new cells (useTool == false is for
        /// automated land changes)
        bool allowLandColourEditing(const std::string& textureFileName, bool useTool = true);

        std::string mBrushTexture;
        int mBrushSize = 1;
        CSVWidget::BrushShape mBrushShape = CSVWidget::BrushShape_Point;
        std::unique_ptr<BrushDraw> mBrushDraw;
        CSVWidget::SceneToolVertexPaintBrush* mVertexPaintBrushScenetool = nullptr;
        int mDragMode = InteractionType_None;
        osg::Group* mParentNode;
        bool mIsEditing = false;
        std::shared_ptr<TerrainSelection> mTerrainShapeSelection;
        int mTotalDiffY = 0;
        std::vector<CSMWorld::CellCoordinates> mAlteredCells;
        osg::Vec3d mEditingPos;
        int mVertexPaintEditTool = VertexPaintEditTool_Replace;
        QColor mVertexPaintEditToolColor;
        int mTargetHeight = 0;

        PagedWorldspaceWidget& getPagedWorldspaceWidget();

    public slots:
        void setBrushSize(int brushSize);
        void setBrushShape(CSVWidget::BrushShape brushShape);
        void setVertexPaintEditTool(int shapeEditTool);
        void setVertexPaintColor(const QColor& color);
    };
}

#endif
