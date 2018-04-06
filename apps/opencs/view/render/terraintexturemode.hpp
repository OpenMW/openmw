#ifndef CSV_RENDER_TERRAINTEXTUREMODE_H
#define CSV_RENDER_TERRAINTEXTUREMODE_H

#include "editmode.hpp"

#include <iostream>
#include <string>

#include <QLabel>
#include <QWidget>
#include <QIcon>
#include <QFrame>
#include <QEvent>

#include <QPushButton>
#include <osg/Geometry>
#include <osg/Vec3>

#include "../widget/modebutton.hpp"
#include "../../model/world/cellcoordinates.hpp"
#include "../../model/world/cell.hpp"
#include "../../model/world/universalid.hpp"
#include "../../model/world/idtable.hpp"

namespace CSVWidget
{
    class SceneToolMode;
}

namespace CSVRender
{

    class BrushWindow : public QWidget {

        Q_OBJECT

        int mButtonSize;
        int mIconSize;
        WorldspaceWidget *mWorldspaceWidget;
        std::string *mBrushTexture;

        public:

            BrushWindow(WorldspaceWidget *worldspaceWidget, QWidget *parent = 0);

            class BrushButton : public QPushButton
            {
                public:
                    std::string *mBrushTexture;
                    BrushButton (const QIcon& icon, const QString& tooltip = "",
                        QWidget *parent = 0);
                    virtual void dragEnterEvent (QDragEnterEvent *event);
                    virtual void dropEvent (QDropEvent *event);
            };

            void configureButtonInitialSettings(BrushButton *button);

        private:
            QLabel *label;

        public slots:

    };


    class TerrainTextureMode : public EditMode
    {
            Q_OBJECT

            CSVWidget::SceneToolMode *mSubMode;
            std::string mSubModeId;
            BrushWindow *brushWindow;

        public:
            std::string mBrushTexture;

            TerrainTextureMode(WorldspaceWidget*, QWidget* parent = nullptr);

            void primarySelectPressed(const WorldspaceHitResult&);
            void secondarySelectPressed(const WorldspaceHitResult&);

            void activate(CSVWidget::SceneToolbar*);
            void deactivate(CSVWidget::SceneToolbar*);
            QIcon drawIconTexture();

            virtual bool primaryEditStartDrag (const QPoint& pos);
            virtual bool secondaryEditStartDrag (const QPoint& pos);
            virtual bool primarySelectStartDrag (const QPoint& pos);
            virtual bool secondarySelectStartDrag (const QPoint& pos);
            virtual void drag (const QPoint& pos, int diffX, int diffY, double speedFactor);
            virtual void dragCompleted(const QPoint& pos);
            virtual void dragAborted();
            virtual void dragWheel (int diff, double speedFactor);
            virtual void dragEnterEvent (QDragEnterEvent *event);
            virtual void dropEvent (QDropEvent *event);
            virtual void dragMoveEvent (QDragMoveEvent *event);

            int getSubModeFromId (const std::string& id) const;
            void subModeChanged (const std::string& id);

        private:

        signals:

        private slots:

    };
}


#endif
