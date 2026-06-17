#ifndef CSV_WIDGET_SCENETOOLTEXTUREBRUSH_H
#define CSV_WIDGET_SCENETOOLTEXTUREBRUSH_H

#include <QFrame>
#include <QGroupBox>

#ifndef Q_MOC_RUN
#include "brushshapes.hpp"
#include "scenetool.hpp"
#endif

#include <components/esm/refid.hpp>

class QTableWidget;
class QDragEnterEvent;
class QDropEvent;
class QHBoxLayout;
class QLabel;
class QModelIndex;
class QObject;
class QPoint;
class QPushButton;
class QSlider;
class QSpinBox;
class QWidget;

namespace CSVRender
{
    class TerrainTextureMode;
}

namespace CSMDoc
{
    class Document;
}

namespace CSVWidget
{
    class SceneToolbar;

    /// \brief Layout-box for some brush button settings
    class BrushSizeControls : public QGroupBox
    {
        Q_OBJECT

    public:
        BrushSizeControls(const QString& title, QWidget* parent);

    private:
        QHBoxLayout* mLayoutSliderSize;
        QSlider* mBrushSizeSlider;
        QSpinBox* mBrushSizeSpinBox;

        friend class SceneToolTextureBrush;
        friend class CSVRender::TerrainTextureMode;
    };

    /// \brief Brush settings window
    class TextureBrushWindow : public QFrame
    {
        Q_OBJECT

    public:
        TextureBrushWindow(CSMDoc::Document& document, QWidget* parent = nullptr);
        void configureButtonInitialSettings(QPushButton* button);

        const QString toolTipPoint = "Paint single point";
        const QString toolTipSquare = "Paint with square brush";
        const QString toolTipCircle = "Paint with circle brush";
        const QString toolTipCustom = "Paint custom selection (not implemented yet)";

    private:
        CSVWidget::BrushShape mBrushShape = CSVWidget::BrushShape_Point;
        int mBrushSize = 1;
        ESM::RefId mBrushTexture;
        CSMDoc::Document& mDocument;
        QLabel* mSelectedBrush;
        QGroupBox* mHorizontalGroupBox;
        std::string mBrushTextureLabel;
        QPushButton* mButtonPoint;
        QPushButton* mButtonSquare;
        QPushButton* mButtonCircle;
        QPushButton* mButtonCustom;
        BrushSizeControls* mSizeSliders;

        friend class SceneToolTextureBrush;
        friend class CSVRender::TerrainTextureMode;

    public slots:
        void setBrushTexture(ESM::RefId brushTexture);
        void setBrushShape();
        void setBrushSize(int brushSize);

    signals:
        void passBrushSize(int brushSize);
        void passBrushShape(CSVWidget::BrushShape brushShape);
        void passTextureId(ESM::RefId brushTexture);
    };

    class SceneToolTextureBrush : public SceneTool
    {
        Q_OBJECT

        QString mToolTip;
        CSMDoc::Document& mDocument;
        QFrame* mPanel;
        QTableWidget* mTable;
        std::vector<ESM::RefId> mBrushHistory;
        TextureBrushWindow* mTextureBrushWindow;

    private:
        void adjustToolTips();

    public:
        SceneToolTextureBrush(SceneToolbar* parent, const QString& toolTip, CSMDoc::Document& document);

        void showPanel(const QPoint& position) override;
        void updatePanel();

        void dropEvent(QDropEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;

        friend class CSVRender::TerrainTextureMode;

    public slots:
        void setButtonIcon(CSVWidget::BrushShape brushShape);
        void updateBrushHistory(ESM::RefId mBrushTexture);
        void clicked(const QModelIndex& index);
        void activate() override;

    signals:
        void passEvent(QDropEvent* event);
        void passEvent(QDragEnterEvent* event);
        void passTextureId(ESM::RefId brushTexture);
    };
}

#endif
