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

            void focusInEvent (QFocusEvent *event) override;

        public:

            SceneToolbar (int buttonSize, QWidget *parent = nullptr);

            /// If insertPoint==0, insert \a tool at the end of the scrollbar. Otherwise
            /// insert tool after \a insertPoint.
            void addTool (SceneTool *tool, SceneTool *insertPoint = nullptr);

            void removeTool (SceneTool *tool);

            int getButtonSize() const;

            int getIconSize() const;

        signals:

            void focusSceneRequest();
    };
}

#endif
