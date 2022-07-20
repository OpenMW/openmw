#ifndef METHODSELECTIONPAGE_HPP
#define METHODSELECTIONPAGE_HPP

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

    private slots:
        void handleBuyButton();
        
    private:
        MainWizard *mWizard;

    };

}

#endif // METHODSELECTIONPAGE_HPP
