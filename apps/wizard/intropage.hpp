#ifndef INTROPAGE_HPP
#define INTROPAGE_HPP

#include <QWizardPage>

#include "ui_intropage.h"

namespace Wizard
{
    class MainWizard;

    class IntroPage : public QWizardPage, private Ui::IntroPage
    {
        Q_OBJECT
    public:
        IntroPage(MainWizard *wizard);

        int nextId() const;

    private:
        MainWizard *mWizard;
    };

}

#endif // INTROPAGE_HPP
