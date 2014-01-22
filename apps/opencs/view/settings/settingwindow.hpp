#ifndef SETTINGWINDOW_HPP
#define SETTINGWINDOW_HPP

#include <QMainWindow>
#include <QList>

namespace CSMSettings { class SettingModel; }
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

        void createPages (CSMSettings::SettingModel &model);

        const PageList &pages() const     { return mPages; }

    public slots:
    };
}

#endif // SETTINGWINDOW_HPP
