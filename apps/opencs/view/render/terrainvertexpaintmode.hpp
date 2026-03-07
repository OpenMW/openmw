#ifndef CSV_RENDER_TERRAINVERTEXPAINTMODE_H
#define CSV_RENDER_TERRAINVERTEXPAINTMODE_H

#include "editmode.hpp"

#include <memory>
#include <string>
#include <utility>

#include <apps/opencs/model/world/cellcoordinates.hpp>

#include "../../model/world/columnimp.hpp"
#include "../widget/brushshapes.hpp"

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
            InteractionType_None
        };

        enum VertexPaintEditTool
        {
            VertexPaintEditTool_Replace = 0
        };

        /// Editmode for terrain vertex colour grid
        TerrainVertexPaintMode(WorldspaceWidget*, osg::Group* parentNode, QWidget* parent = nullptr);

        void primaryOpenPressed(const WorldspaceHitResult& hit) override;

        /// Create single command for one-click vertex paint editing
        void primaryEditPressed(const WorldspaceHitResult& hit) override;

        /// Paint with selected colour on left click
        void primarySelectPressed(const WorldspaceHitResult&) override;

        void activate(CSVWidget::SceneToolbar*) override;
        void deactivate(CSVWidget::SceneToolbar*) override;

        /// Start vertex paint editing command macro
        bool primaryEditStartDrag(const QPoint& pos) override;

        bool secondaryEditStartDrag(const QPoint& pos) override;
        bool primarySelectStartDrag(const QPoint& pos) override;

        /// Handle vertex paint edit behavior during dragging
        void drag(const QPoint& pos, int diffX, int diffY, double speedFactor) override;

        /// End vertex paint editing command macro
        void dragCompleted(const QPoint& pos) override;

        /// Cancel vertex paint editing, and reset all pending changes
        void dragAborted() override;

        void dragWheel(int diff, double speedFactor) override;
        void dragMoveEvent(QDragMoveEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;

    private:
        /// Reset everything in the current edit
        void endVertexPaintEditing();

        /// Handle brush mechanics for colour editing
        void editVertexColourGrid(const std::pair<int, int>& vertexCoords, bool dragOperation);

        /// Alter one pixel's colour
        void alterColour(
            CSMWorld::LandColoursColumn::DataType& landColorsNew, int inCellX, int inCellY, bool useTool = true);

        /// Push terrain vertex colour edits to command macro
        void pushEditToCommand(const CSMWorld::LandColoursColumn::DataType& newLandColours, CSMDoc::Document& document,
            CSMWorld::IdTable& landTable, const std::string& cellId);

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
        int mVertexPaintEditTool = VertexPaintEditTool_Replace;
        QColor mVertexPaintEditToolColor;
        bool mRestoreMode = false;

    public slots:
        void setBrushSize(int brushSize);
        void setBrushShape(CSVWidget::BrushShape brushShape);
        void setVertexPaintEditTool(int shapeEditTool);
        void setVertexPaintColor(const QColor& color);
    };
}

#endif
