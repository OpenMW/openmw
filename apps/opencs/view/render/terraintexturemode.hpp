#ifndef CSV_RENDER_TERRAINTEXTUREMODE_H
#define CSV_RENDER_TERRAINTEXTUREMODE_H

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
    class SceneToolTextureBrush;
}

namespace CSVRender
{
    class PagedWorldspaceWidget;

    /// \brief EditMode for handling the terrain texture editing
    class TerrainTextureMode : public EditMode
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

            /// Editmode for terrain texture grid
            TerrainTextureMode(WorldspaceWidget*, osg::Group* parentNode, QWidget* parent = nullptr);

            void primaryOpenPressed (const WorldspaceHitResult& hit);

            /// \brief Create single command for one-click texture editing
            void primaryEditPressed (const WorldspaceHitResult& hit);

            /// Open brush settings window
            void primarySelectPressed(const WorldspaceHitResult&);

            void secondarySelectPressed(const WorldspaceHitResult&);

            void activate(CSVWidget::SceneToolbar*);
            void deactivate(CSVWidget::SceneToolbar*);

            /// Start texture editing command macro
            virtual bool primaryEditStartDrag (const QPoint& pos);

            virtual bool secondaryEditStartDrag (const QPoint& pos);
            virtual bool primarySelectStartDrag (const QPoint& pos);
            virtual bool secondarySelectStartDrag (const QPoint& pos);

            /// Handle texture edit behavior during dragging
            virtual void drag (const QPoint& pos, int diffX, int diffY, double speedFactor);

            /// End texture editing command macro
            virtual void dragCompleted(const QPoint& pos);

            virtual void dragAborted();
            virtual void dragWheel (int diff, double speedFactor);
            virtual void dragMoveEvent (QDragMoveEvent *event);

            /// Handle brush mechanics, maths regarding worldspace hit etc.
            void editTerrainTextureGrid (const WorldspaceHitResult& hit);

            /// Push texture edits to command macro
            void pushEditToCommand (CSMWorld::LandTexturesColumn::DataType& newLandGrid, CSMDoc::Document& document,
                CSMWorld::IdTable& landTable, std::string cellId);

            /// Create new land texture record from texture asset
            void createTexture(std::string textureFileName);

            /// Create new cell and land if needed
            bool allowLandTextureEditing(std::string textureFileName);

        private:
            std::string mCellId;
            std::string mBrushTexture;
            int mBrushSize;
            int mBrushShape;
            std::vector<std::pair<int, int>> mCustomBrushShape;
            CSVWidget::SceneToolTextureBrush *mTextureBrushScenetool;
            int mDragMode;
            osg::Group* mParentNode;
            bool mIsEditing;
            std::unique_ptr<TerrainSelection> mTerrainTextureSelection;

            const int cellSize {ESM::Land::REAL_SIZE};
            const int landSize {ESM::Land::LAND_SIZE};
            const int landTextureSize {ESM::Land::LAND_TEXTURE_SIZE};

            PagedWorldspaceWidget& getPagedWorldspaceWidget();

        signals:
            void passBrushTexture(std::string brushTexture);

        public slots:
            void handleDropEvent(QDropEvent *event);
            void setBrushSize(int brushSize);
            void setBrushShape(int brushShape);
            void setBrushTexture(std::string brushShape);
    };
}


#endif
