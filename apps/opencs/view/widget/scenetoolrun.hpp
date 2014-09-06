#ifndef CSV_WIDGET_SCENETOOLRUN_H
#define CSV_WIDGET_SCENETOOLRUN_H

#include <vector>
#include <string>

#include "scenetool.hpp"

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

        private:

            void adjustToolTips();

            void updateIcon();

        public:

            SceneToolRun (SceneToolbar *parent, const QString& toolTip, const QString& icon,
                const QString& iconDisabled, const std::vector<std::string>& profiles);

            virtual void showPanel (const QPoint& position);

        signals:

            void runRequest (const std::string& profile);
    };
}

#endif
