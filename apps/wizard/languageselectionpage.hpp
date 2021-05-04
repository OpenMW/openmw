#ifndef LANGUAGESELECTIONPAGE_HPP
#define LANGUAGESELECTIONPAGE_HPP

#include "ui_languageselectionpage.h"

namespace Wizard
{
    class MainWizard;

    class LanguageSelectionPage : public QWizardPage, private Ui::LanguageSelectionPage
    {
        Q_OBJECT
    public:
        LanguageSelectionPage(QWidget *parent);

        int nextId() const override;

    private:
        MainWizard *mWizard;

    protected:
        void initializePage() override;
    };
}

#endif // LANGUAGESELECTIONPAGE_HPP
