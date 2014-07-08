#ifndef CSV_WIDGET_SCENETOOL_H
#define CSV_WIDGET_SCENETOOL_H

#include <QPushButton>

namespace CSVWidget
{
    class SceneToolbar;

    ///< \brief Tool base class
    class SceneTool : public QPushButton
    {
            Q_OBJECT

        public:

            SceneTool (SceneToolbar *parent);

            virtual void showPanel (const QPoint& position) = 0;

        private slots:

            void openRequest();
    };
}

#endif
