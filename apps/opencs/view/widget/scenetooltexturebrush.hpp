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

#include "../../model/doc/document.hpp"

class QTableWidget;

namespace CSVRender
{
    class TerrainTextureMode;
}

namespace CSVWidget
{
    class SceneToolTextureBrush;

    /// \brief Layout-box for some brush button settings
    class BrushSizeControls : public QGroupBox
    {
        Q_OBJECT

        public:
            BrushSizeControls(const QString &title, QWidget *parent);

        private:
            QHBoxLayout *mLayoutSliderSize;
            QSlider *mBrushSizeSlider;
            QSpinBox *mBrushSizeSpinBox;

        friend class SceneToolTextureBrush;
        friend class CSVRender::TerrainTextureMode;
    };

    class SceneToolTextureBrush;

    /// \brief Brush settings window
    class TextureBrushWindow : public QFrame
    {
        Q_OBJECT

        public:
            TextureBrushWindow(CSMDoc::Document& document, QWidget *parent = 0);
            void configureButtonInitialSettings(QPushButton *button);

            const QString toolTipPoint = "Paint single point";
            const QString toolTipSquare = "Paint with square brush";
            const QString toolTipCircle = "Paint with circle brush";
            const QString toolTipCustom = "Paint custom selection (not implemented yet)";

        private:
            int mBrushShape;
            int mBrushSize;
            std::string mBrushTexture;
            CSMDoc::Document& mDocument;
            QLabel *mSelectedBrush;
            QGroupBox *mHorizontalGroupBox;
            std::string mBrushTextureLabel;
            QPushButton *mButtonPoint;
            QPushButton *mButtonSquare;
            QPushButton *mButtonCircle;
            QPushButton *mButtonCustom;
            BrushSizeControls* mSizeSliders;

        friend class SceneToolTextureBrush;
        friend class CSVRender::TerrainTextureMode;

        public slots:
            void setBrushTexture(std::string brushTexture);
            void setBrushShape();
            void setBrushSize(int brushSize);

        signals:
            void passBrushSize (int brushSize);
            void passBrushShape(int brushShape);
            void passTextureId(std::string brushTexture);
    };

    class SceneToolTextureBrush : public SceneTool
    {
            Q_OBJECT

            QString mToolTip;
            CSMDoc::Document& mDocument;
            QFrame *mPanel;
            QTableWidget *mTable;
            std::vector<std::string> mBrushHistory;
            TextureBrushWindow *mTextureBrushWindow;

        private:

            void adjustToolTips();

        public:

            SceneToolTextureBrush (SceneToolbar *parent, const QString& toolTip, CSMDoc::Document& document);

            virtual void showPanel (const QPoint& position);
            void updatePanel ();

            void dropEvent (QDropEvent *event);
            void dragEnterEvent (QDragEnterEvent *event);

        friend class CSVRender::TerrainTextureMode;

        public slots:
            void setButtonIcon(int brushShape);
            void updateBrushHistory (const std::string& mBrushTexture);
            void clicked (const QModelIndex& index);
            virtual void activate();

        signals:
            void passEvent(QDropEvent *event);
            void passEvent(QDragEnterEvent *event);
            void passTextureId(std::string brushTexture);
    };
}

#endif
