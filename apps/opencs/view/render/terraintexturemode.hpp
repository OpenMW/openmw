#ifndef CSV_RENDER_TERRAINTEXTUREMODE_H
#define CSV_RENDER_TERRAINTEXTUREMODE_H

#include "editmode.hpp"

#include <string>

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

namespace CSVWidget
{
    class SceneToolTextureBrush;
}

namespace CSVRender
{

    class TerrainTextureMode : public EditMode
    {
        Q_OBJECT

        public:

            /// \brief Editmode for terrain texture grid
            TerrainTextureMode(WorldspaceWidget*, QWidget* parent = nullptr);

            /// \brief Create single command for one-click texture editing
            void primaryEditPressed (const WorldspaceHitResult& hit);

            /// \brief Open brush settings window
            void primarySelectPressed(const WorldspaceHitResult&);

            void secondarySelectPressed(const WorldspaceHitResult&);

            void activate(CSVWidget::SceneToolbar*);
            void deactivate(CSVWidget::SceneToolbar*);

            /// \brief Start texture editing command macro
            virtual bool primaryEditStartDrag (const QPoint& pos);

            virtual bool secondaryEditStartDrag (const QPoint& pos);
            virtual bool primarySelectStartDrag (const QPoint& pos);
            virtual bool secondarySelectStartDrag (const QPoint& pos);

            /// \brief Handle texture edit behavior during dragging
            virtual void drag (const QPoint& pos, int diffX, int diffY, double speedFactor);

            /// \brief End texture editing command macro
            virtual void dragCompleted(const QPoint& pos);

            virtual void dragAborted();
            virtual void dragWheel (int diff, double speedFactor);
            virtual void dragMoveEvent (QDragMoveEvent *event);

            /// \brief Handle brush mechanics, maths regarding worldspace hit etc.
            void editTerrainTextureGrid (const WorldspaceHitResult& hit);

            /// \brief Push texture edits to command macro
            void pushEditToCommand (CSMWorld::LandTexturesColumn::DataType& newLandGrid, CSMDoc::Document& document,
                CSMWorld::IdTable& landTable, std::string cellId);

            /// \brief Create new land texture record from texture asset
            void createTexture(std::string textureFileName);

            /// \brief Create new cell and land if needed
            bool allowLandTextureEditing(std::string textureFileName);

        private:
            std::string mCellId;
            std::string mBrushTexture;
            int mBrushSize;
            int mBrushShape;
            CSVWidget::SceneToolTextureBrush *mTextureBrushScenetool;

            const int cellSize {ESM::Land::REAL_SIZE};
            const int landSize {ESM::Land::LAND_SIZE};
            const int landTextureSize {ESM::Land::LAND_TEXTURE_SIZE};

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
