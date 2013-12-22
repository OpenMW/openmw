#ifndef SETTINGPAGE_HPP
#define SETTINGPAGE_HPP

#include <QWidget>

#include "settingview.hpp"
#include "support.hpp"
#include "settingbox.hpp"

class QSortFilterProxyModel;

namespace CSMSettings
{
    class SettingModel;
    class SectionFilter;
}

namespace CSVSettings
{
    class SettingBox;

    class SettingPage : public QWidget
    {
        Q_OBJECT

        QList<SettingView *> mViews;
        CSMSettings::SectionFilter *mSectionFilter;
        SettingBox *mBox;

    public:
        explicit SettingPage(const QString &pageName, CSMSettings::SettingModel *model,
                             bool isHoriztonal, QWidget *parent = 0);

        void addView (const CSMSettings::Setting *setting);
        QGroupBox *pageFrame() { return mBox; }
    };
}
#endif // SETTINGPAGE_HPP
