#ifndef USERSETTINGSDIALOG_H
#define USERSETTINGSDIALOG_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidgetItem>
#include <QApplication>
#include <QCheckBox>

#include "../../model/settings/usersettings.hpp"
#include "../../model/settings/support.hpp"

class QStackedWidget;
class QListWidget;
class QDataWidgetMapper;

namespace CSVSettings {

    class AbstractPage;

    class UsdWidget : public QWidget
    {
        QWidget *mWidget;
        QDataWidgetMapper *mMapper;

        Q_OBJECT
    public:

        explicit UsdWidget (QWidget *parent = 0):
            QWidget (parent)
        {

        }

        void setMapper (QDataWidgetMapper *mapper)
        { mMapper = mapper; }

        QDataWidgetMapper *mapper() { return mMapper; }

        void setWidget (QWidget *widget) { mWidget = widget;
                                                     connect (mWidget, SIGNAL (toggled(bool)), this, SIGNAL (toggled(bool)));}
        QWidget *widget() { return mWidget; }

        int index;

    signals:
        void toggled (bool state);
    };

    class UserSettingsDialog : public QMainWindow
    {
        Q_OBJECT

        QListWidget *mListWidget;
        QStackedWidget *mStackedWidget;

        void testMapperRadioButton();
        void testMapperCheckBox();

    public:
        explicit UserSettingsDialog(QMainWindow *parent = 0);

    private:

        /// Settings are written on close
        void closeEvent (QCloseEvent *event);

        /// return the setting page by name
        /// performs dynamic cast to AbstractPage *
        AbstractPage &getAbstractPage (int index);

        void setupStack();
        void createPage (const QString &pageName);

    public slots:

        /// Called when a different page is selected in the left-hand list widget
        void slotChangePage (QListWidgetItem*, QListWidgetItem*);

        void show();

    private slots:

    void slotReadout (bool toggled);

    };

}
#endif // USERSETTINGSDIALOG_H
