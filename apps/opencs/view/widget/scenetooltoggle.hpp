#ifndef CSV_WIDGET_SCENETOOL_TOGGLE_H
#define CSV_WIDGET_SCENETOOL_TOGGLE_H

#include "scenetool.hpp"

#include <map>

class QHBoxLayout;
class QRect;

namespace CSVWidget
{
    class SceneToolbar;
    class PushButton;

    ///< \brief Multi-Toggle tool
    class SceneToolToggle : public SceneTool
    {
            Q_OBJECT

            struct ButtonDesc
            {
                unsigned int mMask;
                std::string mSmallIcon;
                QString mName;
                int mIndex;
            };

            std::string mEmptyIcon;
            QWidget *mPanel;
            QHBoxLayout *mLayout;
            std::map<PushButton *, ButtonDesc> mButtons; // widget, id
            int mButtonSize;
            int mIconSize;
            QString mToolTip;
            PushButton *mFirst;

            void adjustToolTip();

            void adjustIcon();

            QRect getIconBox (int index) const;

        public:

            SceneToolToggle (SceneToolbar *parent, const QString& toolTip,
                const std::string& emptyIcon);

            void showPanel (const QPoint& position) override;

            /// \attention After the last button has been added, setSelection must be called at
            /// least once to finalise the layout.
            ///
            /// \note The layout algorithm can not handle more than 9 buttons. To prevent this An
            /// attempt to add more will result in an exception being thrown.
            /// The small icons will be sized at (x-4)/3 (where x is the main icon size).
            void addButton (const std::string& icon, unsigned int mask,
                const std::string& smallIcon, const QString& name, const QString& tooltip = "");

            unsigned int getSelectionMask() const;

            /// \param or'ed button masks. buttons that do not exist will be ignored.
            void setSelectionMask (unsigned int selection);

        signals:

            void selectionChanged();

        private slots:

            void selected();
    };
}

#endif
