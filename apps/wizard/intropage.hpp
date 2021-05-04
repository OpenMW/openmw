#ifndef INTROPAGE_HPP
#define INTROPAGE_HPP

#include "ui_intropage.h"

namespace Wizard
{
    class MainWizard;

    class IntroPage : public QWizardPage, private Ui::IntroPage
    {
        Q_OBJECT
    public:
        IntroPage(QWidget *parent);

        int nextId() const override;

    private:
        MainWizard *mWizard;
    };

}

#endif // INTROPAGE_HPP
