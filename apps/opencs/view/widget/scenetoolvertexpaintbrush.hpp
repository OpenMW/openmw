#ifndef CSV_WIDGET_SCENETOOLVERTEXPAINTBRUSH_H
#define CSV_WIDGET_SCENETOOLVERTEXPAINTBRUSH_H

#include <QColor>
#include <QFrame>
#include <QGroupBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

#include "brushshapes.hpp"
#include "scenetool.hpp"

class QComboBox;
class QDragEnterEvent;
class QDropEvent;
class QModelIndex;
class QObject;
class QPoint;
class QWidget;

namespace CSMDoc
{
    class Document;
}

namespace CSVRender
{
    class TerrainVertexPaintMode;
}

namespace CSVWidget
{
    class SceneToolbar;

    class ColorButtonWidget : public QPushButton
    {
        Q_OBJECT

    public:
        ColorButtonWidget(QWidget* parent = nullptr);

    private:
        QColor mColor = Qt::white;

        QWidget* findNonPopupParent() const;

    signals:
        void colorChanged(const QColor& newColor);

    private slots:
        void openColorDialog();
    };

    /// \brief Layout-box for some brush button settings
    class VertexPaintBrushSizeControls : public QGroupBox
    {
        Q_OBJECT

    public:
        VertexPaintBrushSizeControls(const QString& title, QWidget* parent);

    private:
        QSlider* mBrushSizeSlider = new QSlider(Qt::Horizontal);
        QSpinBox* mBrushSizeSpinBox = new QSpinBox;

        friend class SceneToolVertexPaintBrush;
        friend class CSVRender::TerrainVertexPaintMode;
    };

    /// \brief Brush settings window
    class VertexPaintBrushWindow : public QFrame
    {
        Q_OBJECT

    public:
        VertexPaintBrushWindow(CSMDoc::Document& document, QWidget* parent = nullptr);
        void configureButtonInitialSettings(QPushButton* button);

        const QString toolTipPoint = "Paint single point";
        const QString toolTipSquare = "Paint with square brush";
        const QString toolTipCircle = "Paint with circle brush";
        const QString toolTipCustom = "Paint with custom brush";

    private:
        CSVWidget::BrushShape mBrushShape = CSVWidget::BrushShape_Point;
        int mBrushSize = 1;
        CSMDoc::Document& mDocument;
        QGroupBox* mHorizontalGroupBox;
        QComboBox* mToolSelector;
        QPushButton* mButtonPoint;
        QPushButton* mButtonSquare;
        QPushButton* mButtonCircle;
        VertexPaintBrushSizeControls* mSizeSliders;
        ColorButtonWidget* mColorButtonWidget;

        friend class SceneToolVertexPaintBrush;
        friend class CSVRender::TerrainVertexPaintMode;

    public slots:
        void setBrushShape();
        void setBrushSize(int brushSize);

    signals:
        void passBrushSize(int brushSize);
        void passBrushShape(CSVWidget::BrushShape brushShape);
    };

    class SceneToolVertexPaintBrush : public SceneTool
    {
        Q_OBJECT

        QString mToolTip;
        CSMDoc::Document& mDocument;
        VertexPaintBrushWindow* mVertexPaintBrushWindow;

    public:
        SceneToolVertexPaintBrush(SceneToolbar* parent, const QString& toolTip, CSMDoc::Document& document);

        void showPanel(const QPoint& position) override;

        friend class CSVRender::TerrainVertexPaintMode;

    public slots:
        void setButtonIcon(CSVWidget::BrushShape brushShape);
        void activate() override;

    signals:
        void passEvent(QDropEvent* event);
        void passEvent(QDragEnterEvent* event);
    };
}

#endif
