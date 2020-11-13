#ifndef CSV_WIDGET_PUSHBUTTON_H
#define CSV_WIDGET_PUSHBUTTON_H

#include <QPushButton>

namespace CSMPrefs
{
    class Setting;
}

namespace CSVWidget
{
    class PushButton : public QPushButton
    {
            Q_OBJECT

        public:

            enum Type
            {
                Type_TopMode, // top level button for mode selector panel
                Type_TopAction, // top level button that triggers an action
                Type_Mode, // mode button
                Type_Toggle
            };

        private:

            bool mKeepOpen;
            Type mType;
            QString mToolTip;
            QString mProcessedToolTip;

        private:

            void processShortcuts();
            void setExtendedToolTip();

        protected:

            void keyPressEvent (QKeyEvent *event) override;

            void keyReleaseEvent (QKeyEvent *event) override;

            void mouseReleaseEvent (QMouseEvent *event) override;

        public:

            /// \param push Do not maintain a toggle state
            PushButton (const QIcon& icon, Type type, const QString& tooltip = "",
                QWidget *parent = nullptr);

            /// \param push Do not maintain a toggle state
            PushButton (Type type, const QString& tooltip = "",
                QWidget *parent = nullptr);

            bool hasKeepOpen() const;

            /// Return tooltip used at construction (without any button-specific modifications)
            QString getBaseToolTip() const;

            Type getType() const;

        private slots:

            void checkedStateChanged (bool checked);
            void settingChanged (const CSMPrefs::Setting *setting);
    };
}

#endif
