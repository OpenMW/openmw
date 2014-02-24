#ifndef SETTINGWINDOW_HPP
#define SETTINGWINDOW_HPP

#include <QMainWindow>
#include <QList>
#include <QSortFilterProxyModel>

#include "support.hpp"

class QStandardItemModel;

namespace CSMSettings { class SettingManager; }

namespace CSVSettings {

    class Page;

    typedef QList <Page *> PageList;

    class SettingWindow : public QMainWindow
    {
        Q_OBJECT

        PageList mPages;

    public:
        explicit SettingWindow(QWidget *parent = 0);

    protected:

        virtual void closeEvent (QCloseEvent *event);

        void createPages (CSMSettings::SettingManager &manager);

        const PageList &pages() const     { return mPages; }

        QSortFilterProxyModel *buildFilter (QAbstractItemModel &model,
                                            CSMSettings::SettingProperty column,
                                            const QString &expression);
    public slots:
    };
}

#endif // SETTINGWINDOW_HPP
