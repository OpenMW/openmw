#ifndef USERSETTINGSDIALOG_H
#define USERSETTINGSDIALOG_H

#include <QMainWindow>
#include <QStackedWidget>

#include <components/files/configurationmanager.hpp>
#include "../../model/settings/usersettings.hpp"
#include "../../model/settings/support.hpp"

class QHBoxLayout;
class AbstractWidget;
class QStackedWidget;
class QListWidget;

namespace CSVSettings {

    class AbstractPage;
    class UserSettingsDialog : public QMainWindow
    {
        Q_OBJECT

        QStringList mPaths;
        QListWidget *mListWidget;
        QStackedWidget *mStackedWidget;
        CSMSettings::UserSettings mUserSettings;
        Files::ConfigurationManager mCfgMgr;

    public:
        UserSettingsDialog(QMainWindow *parent = 0);
        ~UserSettingsDialog();

    private:

        void closeEvent (QCloseEvent *event);
        AbstractPage *getAbstractPage (int index);
        void setWidgetStates (CSMSettings::SectionMap settingsMap);
        void buildPages();
        void positionWindow ();
        CSMSettings::SectionMap loadSettings();
        void writeSettings();
        void createSamplePage();

        template <typename T>
        void createPage (const QString &title)
        {
            T *page = new T(title, this);

            mStackedWidget->addWidget (dynamic_cast<QWidget *>(page));

            new QListWidgetItem (page->objectName(), mListWidget);

            //finishing touches
            if (mStackedWidget->sizeHint().width() < 640)
                mStackedWidget->sizeHint().setWidth(640);

            if (mStackedWidget->sizeHint().height() < 480)
                mStackedWidget->sizeHint().setHeight(480);

            resize (mStackedWidget->sizeHint());
        }

    signals:
        void signalUpdateEditorSetting (const QString &settingName, const QString &settingValue);

    public slots:
        void slotChangePage (QListWidgetItem*, QListWidgetItem*);
    };

}
#endif // USERSETTINGSDIALOG_H
