#ifndef USERSETTINGSDIALOG_H
#define USERSETTINGSDIALOG_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidgetItem>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

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

        QListWidget *mListWidget;
        QStackedWidget *mStackedWidget;
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
        void writeSettings();
        void createSamplePage();

        //Pages
        void createWindowPage();

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

    public slots:
        void slotChangePage (QListWidgetItem*, QListWidgetItem*);
    };

}
#endif // USERSETTINGSDIALOG_H
