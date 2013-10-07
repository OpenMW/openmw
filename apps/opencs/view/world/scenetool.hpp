#ifndef CSV_WORLD_SCENETOOL_H
#define CSV_WORLD_SCENETOOL_H

#include <QPushButton>

namespace CSVWorld
{
    class SceneToolbar;

    ///< \brief Tool base class
    class SceneTool : public QPushButton
    {
            Q_OBJECT

        public:

            SceneTool (SceneToolbar *parent);

            virtual void showPanel (const QPoint& position) = 0;

        protected slots:

            void updateIcon (const QIcon& icon);

        private slots:

            void openRequest();
    };
}

#endif
