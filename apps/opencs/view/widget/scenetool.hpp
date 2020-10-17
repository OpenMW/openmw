#ifndef CSV_WIDGET_SCENETOOL_H
#define CSV_WIDGET_SCENETOOL_H

#include "pushbutton.hpp"

namespace CSVWidget
{
    class SceneToolbar;

    ///< \brief Tool base class
    class SceneTool : public PushButton
    {
            Q_OBJECT

        public:

            SceneTool (SceneToolbar *parent, Type type = Type_TopMode);

            virtual void showPanel (const QPoint& position) = 0;

            /// This function will only called for buttons of type Type_TopAction. The default
            /// implementation is empty.
            virtual void activate();

        protected:

            void mouseReleaseEvent (QMouseEvent *event) override;

        private slots:

            void openRequest();
    };
}

#endif
