#ifndef SETTINGPAGE_HPP
#define SETTINGPAGE_HPP

#include <QWidget>

#include "settingview.hpp"

class QSortFilterProxyModel;

namespace CSVSettings
{
    class SettingBox;

    class SettingPage : public QWidget
    {
        Q_OBJECT

        QList<SettingView *> mViews;
        QSortFilterProxyModel *mSectionFilter;
        SettingBox *mBox;

    public:
        explicit SettingPage(const QString &pageName, QWidget *parent = 0);

        void addView (WidgetType widgType, const QString &viewName);

        template <typename T>
        SettingView *createView (const QString &viewName)
        {
            return new SettingView<T> (viewName, mSectionFilter, this);
        }
    };
}
#endif // SETTINGPAGE_HPP
