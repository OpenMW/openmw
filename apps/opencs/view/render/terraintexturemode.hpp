#ifndef CSV_RENDER_TERRAINTEXTUREMODE_H
#define CSV_RENDER_TERRAINTEXTUREMODE_H

#include "editmode.hpp"

#include <string>

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QGroupBox>
#include <QSlider>
#include <QIcon>
#include <QFrame>
#include <QEvent>
#include <QHBoxLayout>
#include <QPushButton>

#include "../../model/world/data.hpp"
#include "../../model/world/land.hpp"

#include "../../model/doc/document.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/landtexture.hpp"

namespace CSVWidget
{
    class SceneToolMode;
}

namespace CSVRender
{
    class BrushSizeControls : public QGroupBox
    {
        Q_OBJECT

        public:
            BrushSizeControls(const QString &title, QWidget *parent);
            QSlider *brushSizeSlider;

        private:
            QSpinBox *brushSizeSpinBox;
            QHBoxLayout *layoutSliderSize;
    };

    class TextureBrushButton : public QPushButton
    {
        Q_OBJECT

        public:
            TextureBrushButton (const QIcon& icon, const QString& tooltip = "",
                QWidget *parent = 0);
    };

    class TextureBrushWindow : public QFrame
    {
        Q_OBJECT

        public:
            TextureBrushWindow(WorldspaceWidget *worldspaceWidget, QWidget *parent = 0);
            void configureButtonInitialSettings(TextureBrushButton *button);

            TextureBrushButton *buttonPoint = new TextureBrushButton(QIcon (QPixmap (":scenetoolbar/brush-point")), "", this);
            TextureBrushButton *buttonSquare = new TextureBrushButton(QIcon (QPixmap (":scenetoolbar/brush-square")), "", this);
            TextureBrushButton *buttonCircle = new TextureBrushButton(QIcon (QPixmap (":scenetoolbar/brush-circle")), "", this);
            TextureBrushButton *buttonCustom = new TextureBrushButton(QIcon (QPixmap (":scenetoolbar/brush-custom")), "", this);

        private:
            QLabel *selectedBrush;
            QGroupBox *horizontalGroupBox;
            WorldspaceWidget *mWorldspaceWidget;
            int mBrushSize;
            int mBrushShape;
            std::string mBrushTexture;
            std::string mBrushTextureLabel;

        public slots:
            void setBrushTexture(std::string brushTexture);
            void setBrushShape();
            void setBrushSize(int brushSize);

        signals:
            void passBrushSize (int brushSize);
            void passBrushShape(int brushShape);
    };

    class TerrainTextureMode : public EditMode
    {
        Q_OBJECT

        public:

            TerrainTextureMode(WorldspaceWidget*, QWidget* parent = nullptr);

            void primaryEditPressed (const WorldspaceHitResult& hit);

            void primarySelectPressed(const WorldspaceHitResult&);
            void secondarySelectPressed(const WorldspaceHitResult&);

            void activate(CSVWidget::SceneToolbar*);
            void deactivate(CSVWidget::SceneToolbar*);

            virtual bool primaryEditStartDrag (const QPoint& pos);
            virtual bool secondaryEditStartDrag (const QPoint& pos);
            virtual bool primarySelectStartDrag (const QPoint& pos);
            virtual bool secondarySelectStartDrag (const QPoint& pos);
            virtual void drag (const QPoint& pos, int diffX, int diffY, double speedFactor);
            virtual void dragCompleted(const QPoint& pos);
            virtual void dragAborted();
            virtual void dragWheel (int diff, double speedFactor);
            virtual void dragMoveEvent (QDragMoveEvent *event);

            void editTerrainTextureGrid (const WorldspaceHitResult& hit);
            void pushEditToCommand (CSMWorld::LandTexturesColumn::DataType& newLandGrid, CSMDoc::Document& document,
                CSMWorld::IdTable& landTable, std::string cellId);

        private:
            TextureBrushWindow *textureBrushWindow;
            std::string mCellId;
            std::string mBrushTexture;
            int mBrushSize;
            int mBrushShape;

            const int cellSize {ESM::Land::REAL_SIZE};
            const int landSize {ESM::Land::LAND_SIZE};
            const int landTextureSize {ESM::Land::LAND_TEXTURE_SIZE};

        signals:
            void passBrushTexture(std::string brushTexture);

        public slots:
            void handleDragEnterEvent (QDragEnterEvent *event);
            void handleDropEvent(QDropEvent *event);
            void setBrushSize(int brushSize);
            void setBrushShape(int brushShape);

    };
}


#endif
