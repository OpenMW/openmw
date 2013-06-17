#ifndef USERSETTINGSDIALOG_H
#define USERSETTINGSDIALOG_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidgetItem>
#include <QApplication>

#include "../../model/settings/usersettings.hpp"
#include "../../model/settings/support.hpp"

#include "editorpage.hpp"

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

    public:
        UserSettingsDialog(QMainWindow *parent = 0);
        ~UserSettingsDialog();

    private:

        void closeEvent (QCloseEvent *event);
        AbstractPage *getAbstractPage (int index);
        void setWidgetStates ();
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

            QFontMetrics fm (QApplication::font());
            int textWidth = fm.width(page->objectName());

            if ((textWidth + 50) > mListWidget->minimumWidth())
                mListWidget->setMinimumWidth(textWidth + 50);

            resize (mStackedWidget->sizeHint());
        }

    public slots:
        void slotChangePage (QListWidgetItem*, QListWidgetItem*);
    };

}
#endif // USERSETTINGSDIALOG_H
