#ifndef CSV_WIDGET_SCENETOOL_TOGGLE2_H
#define CSV_WIDGET_SCENETOOL_TOGGLE2_H

#include "scenetool.hpp"

#include <map>

class QHBoxLayout;
class QRect;

namespace CSVWidget
{
    class SceneToolbar;
    class PushButton;

    ///< \brief Multi-Toggle tool
    ///
    /// Top level button is using predefined icons instead building a composite icon.
    class SceneToolToggle2 : public SceneTool
    {
            Q_OBJECT

            struct ButtonDesc
            {
                unsigned int mId;
                QString mName;
                int mIndex;
            };

            std::string mCompositeIcon;
            std::string mSingleIcon;
            QWidget *mPanel;
            QHBoxLayout *mLayout;
            std::map<PushButton *, ButtonDesc> mButtons; // widget, id
            int mButtonSize;
            int mIconSize;
            QString mToolTip;
            PushButton *mFirst;

            void adjustToolTip();

            void adjustIcon();

        public:

            /// The top level icon is compositeIcon + sum of bitpatterns for active buttons (in
            /// decimal)
            ///
            /// The icon for individual toggle buttons is signleIcon + bitmask for button (in
            /// decimal)
            SceneToolToggle2 (SceneToolbar *parent, const QString& toolTip,
                const std::string& compositeIcon, const std::string& singleIcon);

            virtual void showPanel (const QPoint& position);

            /// \attention After the last button has been added, setSelection must be called at
            /// least once to finalise the layout.
            void addButton (unsigned int id,
                const QString& name, const QString& tooltip = "", bool disabled = false);

            unsigned int getSelection() const;

            /// \param or'ed button IDs. IDs that do not exist will be ignored.
            void setSelection (unsigned int selection);

        signals:

            void selectionChanged();

        private slots:

            void selected();
    };
}

#endif
