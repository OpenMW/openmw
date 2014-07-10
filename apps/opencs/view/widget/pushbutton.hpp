#ifndef CSV_WIDGET_PUSHBUTTON_H
#define CSV_WIDGET_PUSHBUTTON_H

#include <QPushButton>

namespace CSVWidget
{
    class PushButton : public QPushButton
    {
            Q_OBJECT

        public:

            enum Type
            {
                Type_TopMode, // top level button for mode selector panel
                Type_Mode // mode button
            };

        private:

            bool mKeepOpen;
            Type mType;

        private:

            void setExtendedToolTip (const std::string& text);

        protected:

            virtual void keyPressEvent (QKeyEvent *event);

            virtual void keyReleaseEvent (QKeyEvent *event);

            virtual void mouseReleaseEvent (QMouseEvent *event);

        public:

            /// \param push Do not maintain a toggle state
            PushButton (const QIcon& icon, Type type, const std::string& tooltip = "",
                QWidget *parent = 0);

            /// \param push Do not maintain a toggle state
            PushButton (Type type, const std::string& tooltip = "",
                QWidget *parent = 0);

            bool hasKeepOpen() const;
    };
}

#endif
