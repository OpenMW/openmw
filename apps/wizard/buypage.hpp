#ifndef BUYPAGE_HPP
#define BUYPAGE_HPP

#include <QWizardPage>

#include "ui_buypage.h"

namespace Wizard
{
    class MainWizard;

    class BuyPage : public QWizardPage, private Ui::BuyPage
    {
        Q_OBJECT
    public:
        BuyPage(QWidget *parent);

        int nextId() const;

    private:
        MainWizard *mWizard;
    };
}

#endif // BUYPAGE_HPP
