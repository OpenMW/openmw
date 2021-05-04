#ifndef CONCLUSIONPAGE_HPP
#define CONCLUSIONPAGE_HPP

#include "ui_conclusionpage.h"

namespace Wizard
{
    class MainWizard;

    class ConclusionPage : public QWizardPage, private Ui::ConclusionPage
    {
        Q_OBJECT
    public:
        ConclusionPage(QWidget *parent);

        int nextId() const override;

    private:
        MainWizard *mWizard;

    protected:
        void initializePage() override;

    };

}

#endif // CONCLUSIONPAGE_HPP
