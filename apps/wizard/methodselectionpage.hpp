#ifndef METHODSELECTIONPAGE_HPP
#define METHODSELECTIONPAGE_HPP

#include <QWizardPage>

#include "ui_methodselectionpage.h"

namespace Wizard
{

    class MethodSelectionPage : public QWizardPage, private Ui::MethodSelectionPage
    {
        Q_OBJECT
    public:
        MethodSelectionPage(QWidget *parent = 0);

        int nextId() const;

    };

}

#endif // METHODSELECTIONPAGE_HPP
