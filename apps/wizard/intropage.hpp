#ifndef INTROPAGE_HPP
#define INTROPAGE_HPP

#include <QWizardPage>

#include "ui_intropage.h"

namespace Wizard
{

    class IntroPage : public QWizardPage, private Ui::IntroPage
    {
        Q_OBJECT
    public:
        IntroPage(QWidget *parent = 0);

    };

}

#endif // INTROPAGE_HPP
