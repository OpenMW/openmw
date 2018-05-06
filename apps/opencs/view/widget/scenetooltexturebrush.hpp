#ifndef CSV_WIDGET_SCENETOOLTEXTUREBRUSH_H
#define CSV_WIDGET_SCENETOOLTEXTUREBRUSH_H

#include <QIcon>
#include <QFrame>
#include <QModelIndex>

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QGroupBox>
#include <QSlider>
#include <QEvent>
#include <QHBoxLayout>
#include <QPushButton>

#include "scenetool.hpp"

/*namespace CSVRender
{
    class TerrainTextureMode;
}*/

namespace CSVWidget
{
    /// \brief Layout-box for some brush button settings
    class BrushSizeControls : public QGroupBox
    {
        Q_OBJECT

        public:
            BrushSizeControls(const QString &title, QWidget *parent);
            QSlider *mBrushSizeSlider;

        private:
            QSpinBox *mBrushSizeSpinBox;
            QHBoxLayout *mLayoutSliderSize;
    };

    /// \brief Brush settings window
    class TextureBrushWindow : public QFrame
    {
        Q_OBJECT

        public:
            TextureBrushWindow(QWidget *parent = 0);
            void configureButtonInitialSettings(QPushButton *button);

            QPushButton *mButtonPoint = new QPushButton(QIcon (QPixmap (":scenetoolbar/brush-point")), "", this);
            QPushButton *mButtonSquare = new QPushButton(QIcon (QPixmap (":scenetoolbar/brush-square")), "", this);
            QPushButton *mButtonCircle = new QPushButton(QIcon (QPixmap (":scenetoolbar/brush-circle")), "", this);
            QPushButton *mButtonCustom = new QPushButton(QIcon (QPixmap (":scenetoolbar/brush-custom")), "", this);
            BrushSizeControls* mSizeSliders = new BrushSizeControls(tr(""), this);
            int mBrushShape;

        private:
            QLabel *mSelectedBrush;
            QGroupBox *mHorizontalGroupBox;
            int mBrushSize;
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

    class SceneToolTextureBrush : public SceneTool
    {
            Q_OBJECT

            QString mToolTip;

        private:

            void adjustToolTips();

        public:

            SceneToolTextureBrush (SceneToolbar *parent, const QString& toolTip);

            virtual void showPanel (const QPoint& position);

            TextureBrushWindow *mTextureBrushWindow;

            void dropEvent (QDropEvent *event);
            void dragEnterEvent (QDragEnterEvent *event);

        public slots:
            void setButtonIcon(int brushShape);
            virtual void activate();

        signals:
            void passEvent(QDropEvent *event);
            void passEvent(QDragEnterEvent *event);
    };
}

#endif
