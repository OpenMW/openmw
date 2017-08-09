#ifndef CONCLUSIONPAGE_HPP
#define CONCLUSIONPAGE_HPP

#include <QWizardPage>

#include "ui_conclusionpage.h"

namespace Wizard
{
    class MainWizard;

    class ConclusionPage : public QWizardPage, private Ui::ConclusionPage
    {
        Q_OBJECT
    public:
        ConclusionPage(QWidget *parent);

        int nextId() const;

    private:
        MainWizard *mWizard;

    protected:
        void initializePage();

    };

}

#endif // CONCLUSIONPAGE_HPP
