#ifndef METHODSELECTIONPAGE_HPP
#define METHODSELECTIONPAGE_HPP

#include <QWizardPage>

#include "ui_methodselectionpage.h"

namespace Wizard
{
    class MainWizard;

    class MethodSelectionPage : public QWizardPage, private Ui::MethodSelectionPage
    {
        Q_OBJECT
    public:
        MethodSelectionPage(QWidget *parent);

        int nextId() const override;

    private:
        MainWizard *mWizard;

    };

}

#endif // METHODSELECTIONPAGE_HPP
