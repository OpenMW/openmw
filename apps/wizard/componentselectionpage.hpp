#ifndef COMPONENTSELECTIONPAGE_HPP
#define COMPONENTSELECTIONPAGE_HPP

#include <QWizardPage>

#include "ui_componentselectionpage.h"

namespace Wizard
{
    class MainWizard;

    class ComponentSelectionPage : public QWizardPage, private Ui::ComponentSelectionPage
    {
        Q_OBJECT
    public:
        ComponentSelectionPage(QWidget *parent);

        int nextId() const;
        virtual bool validatePage();

    private slots:
        void updateButton(QListWidgetItem *item);

    private:
        MainWizard *mWizard;

    protected:
        void initializePage();

    };

}

#endif // COMPONENTSELECTIONPAGE_HPP
