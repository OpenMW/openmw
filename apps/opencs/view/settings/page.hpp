#ifndef CSVSETTINGS_PAGE_HPP
#define CSVSETTINGS_PAGE_HPP

#include <QWidget>

#include "support.hpp"
#include "settingbox.hpp"
#include "../../model/settings/setting.hpp"

class QGroupBox;

namespace CSMSettings { class Selector; }
namespace CSVSettings
{
    class View;
    class SettingBox;
    class IViewFactory;
    class SettingWindow;

    class Page : public QWidget
    {
        Q_OBJECT

        QList<View *> mViews;
        CSMSettings::SettingList mSettingList;

        SettingWindow *mParent;
        SettingBox *mBox;
        static QMap <ViewType, IViewFactory *> mViewFactories;

    public:
        explicit Page(const QString &pageName,
                      CSMSettings::SettingList settingList,
                      SettingWindow *parent);

        void addView (CSMSettings::Setting *setting);
        View *findView (const QString &page, const QString &setting) const;

        QGroupBox *pageFrame() { return mBox; }

    private:

        void setupPage ();
        void setupViews (CSMSettings::SettingList &settingList);
        void buildFactories();
        const CSMSettings::Setting *findSetting (const QString &settingName);
    };
}
#endif // CSVSETTINGS_PAGE_HPP
