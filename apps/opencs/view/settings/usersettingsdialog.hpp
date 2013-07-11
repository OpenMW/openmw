#ifndef USERSETTINGSDIALOG_H
#define USERSETTINGSDIALOG_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidgetItem>
#include <QApplication>

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

    public:
        UserSettingsDialog(QMainWindow *parent = 0);
        ~UserSettingsDialog();

    private:

        /// Settings are written on close
        void closeEvent (QCloseEvent *event);

        /// return the setting page by name
        /// performs dynamic cast to AbstractPage *
        AbstractPage &getAbstractPage (int index);
        void setWidgetStates ();
        void buildPages();
        void writeSettings();

        /// Templated function to create a custom user preference page
        template <typename T>
        void createPage ()
        {
            T *page = new T(mStackedWidget);

            mStackedWidget->addWidget (&dynamic_cast<QWidget &>(*page));

            new QListWidgetItem (page->objectName(), mListWidget);

            //finishing touches
            QFontMetrics fm (QApplication::font());
            int textWidth = fm.width(page->objectName());

            if ((textWidth + 50) > mListWidget->minimumWidth())
                mListWidget->setMinimumWidth(textWidth + 50);

            resize (mStackedWidget->sizeHint());
        }

    public slots:

        /// Called when a different page is selected in the left-hand list widget
        void slotChangePage (QListWidgetItem*, QListWidgetItem*);
    };

}
#endif // USERSETTINGSDIALOG_H
