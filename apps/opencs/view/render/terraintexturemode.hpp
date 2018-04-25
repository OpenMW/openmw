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

        private:
            QSlider *brushSizeSlider;
            QSpinBox *brushSizeSpinBox;
            QHBoxLayout *layoutSliderSize;
    };

    class TextureBrushWindow : public QFrame
    {
        Q_OBJECT

        public:
            TextureBrushWindow(WorldspaceWidget *worldspaceWidget, QWidget *parent = 0);
            void configureButtonInitialSettings(QPushButton *button);

        private:
            QLabel *selectedBrush;
            QGroupBox *horizontalGroupBox;
            int mButtonSize;
            int mIconSize;
            WorldspaceWidget *mWorldspaceWidget;
            std::string mBrushTexture;
            std::string mBrushTextureLabel;

        public slots:
            void getBrushTexture(std::string brushTexture);
    };

    class TerrainTextureMode : public EditMode
    {
        Q_OBJECT

        public:
            std::string mBrushTexture;

            TerrainTextureMode(WorldspaceWidget*, QWidget* parent = nullptr);

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

        private:
            TextureBrushWindow *textureBrushWindow;

        signals:

            void passBrushTexture(std::string brushTexture);

        public slots:
            void handleDragEnterEvent (QDragEnterEvent *event);
            void handleDropEvent(QDropEvent *event);
    };
}


#endif
