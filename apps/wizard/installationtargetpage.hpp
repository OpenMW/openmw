#ifndef INSTALLATIONTARGETPAGE_HPP
#define INSTALLATIONTARGETPAGE_HPP

#include <QWizardPage>

#include "ui_installationtargetpage.h"

namespace Wizard
{

    class InstallationTargetPage : public QWizardPage, private Ui::InstallationTargetPage
    {
        Q_OBJECT
    public:
        InstallationTargetPage(QWidget *parent = 0);

        int nextId() const;

    };

}

#endif // INSTALLATIONTARGETPAGE_HPP
