#ifndef CSV_WIDGET_SCENETOOLBAR_H
#define CSV_WIDGET_SCENETOOLBAR_H

#include <QWidget>

class QVBoxLayout;

namespace CSVWidget
{
    class SceneTool;

    class SceneToolbar : public QWidget
    {
            Q_OBJECT

            QVBoxLayout *mLayout;
            int mButtonSize;
            int mIconSize;

        protected:

            virtual void focusInEvent (QFocusEvent *event);

        public:

            SceneToolbar (int buttonSize, QWidget *parent = 0);

            /// If insertPoint==0, insert \a tool at the end of the scrollbar. Otherwise
            /// insert tool after \a insertPoint.
            void addTool (SceneTool *tool, SceneTool *insertPoint = 0);

            void removeTool (SceneTool *tool);

            int getButtonSize() const;

            int getIconSize() const;

        signals:

            void focusSceneRequest();
    };
}

#endif
