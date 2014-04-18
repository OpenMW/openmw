#ifndef INSTALLATIONTARGETPAGE_HPP
#define INSTALLATIONTARGETPAGE_HPP

#include <QWizardPage>

#include "ui_installationtargetpage.h"

namespace Files
{
    struct ConfigurationManager;
}

namespace Wizard
{
    class MainWizard;

    class InstallationTargetPage : public QWizardPage, private Ui::InstallationTargetPage
    {
        Q_OBJECT
    public:
        InstallationTargetPage(QWidget *parent, const Files::ConfigurationManager &cfg);

        int nextId() const;
        virtual bool validatePage();

    private slots:
        void on_browseButton_clicked();

    private:
        MainWizard *mWizard;
        const Files::ConfigurationManager &mCfgMgr;

    protected:
        void initializePage();

    };

}

#endif // INSTALLATIONTARGETPAGE_HPP
