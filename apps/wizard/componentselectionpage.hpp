#ifndef COMPONENTSELECTIONPAGE_HPP
#define COMPONENTSELECTIONPAGE_HPP

#include <QWizardPage>

#include "ui_componentselectionpage.h"

namespace Wizard
{

    class ComponentSelectionPage : public QWizardPage, private Ui::ComponentSelectionPage
    {
        Q_OBJECT
    public:
        ComponentSelectionPage(QWidget *parent = 0);

        int nextId() const;

    };

}

#endif // COMPONENTSELECTIONPAGE_HPP
