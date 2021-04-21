#ifndef COMPONENTSELECTIONPAGE_HPP
#define COMPONENTSELECTIONPAGE_HPP

#include "ui_componentselectionpage.h"

namespace Wizard
{
    class MainWizard;

    class ComponentSelectionPage : public QWizardPage, private Ui::ComponentSelectionPage
    {
        Q_OBJECT
    public:
        ComponentSelectionPage(QWidget *parent);

        int nextId() const override;
        bool validatePage() override;

    private slots:
        void updateButton(QListWidgetItem *item);

    private:
        MainWizard *mWizard;

    protected:
        void initializePage() override;

    };

}

#endif // COMPONENTSELECTIONPAGE_HPP
