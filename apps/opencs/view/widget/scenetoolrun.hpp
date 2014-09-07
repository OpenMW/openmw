#ifndef CSV_WIDGET_SCENETOOLRUN_H
#define CSV_WIDGET_SCENETOOLRUN_H

#include <vector>
#include <string>

#include "scenetool.hpp"

class QFrame;
class QTableWidget;
class QModelIndex;

namespace CSVWidget
{
    class SceneToolRun : public SceneTool
    {
            Q_OBJECT

            std::vector<std::string> mProfiles;
            int mCurrentIndex;
            QString mToolTip;
            QString mIcon;
            QString mIconDisabled;
            QFrame *mPanel;
            QTableWidget *mTable;

        private:

            void adjustToolTips();

            void updateIcon();

            void updatePanel();

        public:

            SceneToolRun (SceneToolbar *parent, const QString& toolTip, const QString& icon,
                const QString& iconDisabled, const std::vector<std::string>& profiles);

            virtual void showPanel (const QPoint& position);

            virtual void activate();

            /// \attention This function does not remove the profile from the profile selection
            /// panel.
            void removeProfile (const std::string& profile);

        private slots:

            void clicked (const QModelIndex& index);

        signals:

            void runRequest (const std::string& profile);
    };
}

#endif
