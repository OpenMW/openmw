#ifndef CSVSETTINGS_PAGE_HPP
#define CSVSETTINGS_PAGE_HPP

#include <QSizePolicy>
#include <QWidget>
#include <QMap>
#include <QList>

#include "frame.hpp"

#include "../../model/settings/support.hpp"

namespace CSMSettings { class Setting; }

namespace CSVSettings
{
    class View;
    class IViewFactory;
    class SettingWindow;

    class Page : public Frame
    {
        Q_OBJECT

        QList<View *> mViews;
        SettingWindow *mParent;
        static QMap <ViewType, IViewFactory *> mViewFactories;
        bool mIsEditorPage;

    public:
        explicit Page(const QString &pageName,
                      QList <CSMSettings::Setting *> settingList,
                      SettingWindow *parent);

        ///Creates a new view based on the passed setting and adds it to
        ///the page.
        void addView (CSMSettings::Setting *setting);

        ///Iterates the views created for this page based on the passed setting
        ///and returns it.
        View *findView (const QString &page, const QString &setting) const;

        const QList <View *> &views () const              { return mViews; }

    private:

        ///Creates views based on the passed setting list
        void setupViews (QList <CSMSettings::Setting *> &settingList);

        ///Creates factory objects for view construction
        void buildFactories();
    };
}
#endif // CSVSETTINGS_PAGE_HPP
