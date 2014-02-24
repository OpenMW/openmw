#ifndef CSVSETTINGS_PAGE_HPP
#define CSVSETTINGS_PAGE_HPP

#include <QWidget>

#include "support.hpp"
#include "settingbox.hpp"
#include <QAbstractItemModel>

class QSortFilterProxyModel;
class QGroupBox;

namespace CSMSettings
{
    class Setting;
}

namespace CSVSettings
{
    class View;
    class SettingBox;
    class IViewFactory;

    class Page : public QWidget
    {
        Q_OBJECT

        QList<View *> mViews;
        SettingBox *mBox;
        static QMap <ViewType, IViewFactory *> mViewFactories;

    public:
        explicit Page(const QString &pageName,
                      const QList <CSMSettings::Setting> &settingList,
                                                        QWidget *parent = 0);

        void addView (const CSMSettings::Setting &setting);

        QGroupBox *pageFrame() { return mBox; }

    private:

        void setupPage ();
        void setupViews (const QList <CSMSettings::Setting> &settingList);

        void buildFactories();
    };
}
#endif // CSVSETTINGS_PAGE_HPP
