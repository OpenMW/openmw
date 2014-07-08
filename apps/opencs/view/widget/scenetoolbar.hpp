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

        public:

            SceneToolbar (int buttonSize, QWidget *parent = 0);

            void addTool (SceneTool *tool);

            int getButtonSize() const;

            int getIconSize() const;
    };
}

#endif
