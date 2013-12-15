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
        ComponentSelectionPage(MainWizard *wizard);

        int nextId() const;

    private slots:
        void updateButton(QListWidgetItem *item);
        void debugMe(QString &text);

    private:
        MainWizard *mWizard;

    protected:
        void initializePage();

    };

}

#endif // COMPONENTSELECTIONPAGE_HPP
