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

namespace CSMSettings { class BinaryWidgetModel; }
namespace CSVSettings {

    class AbstractPage;

    class UserSettingsDialog : public QMainWindow
    {
        Q_OBJECT

        QListWidget *mListWidget;
        QStackedWidget *mStackedWidget;
        CSMSettings::BinaryWidgetModel *mBinModel;

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
