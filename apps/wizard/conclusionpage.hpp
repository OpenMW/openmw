#ifndef CONCLUSIONPAGE_HPP
#define CONCLUSIONPAGE_HPP

#include <QWizardPage>

#include "ui_conclusionpage.h"

namespace Wizard
{

    class ConclusionPage : public QWizardPage, private Ui::ConclusionPage
    {
        Q_OBJECT
    public:
        ConclusionPage(QWidget *parent = 0);

        int nextId() const;

    };

}

#endif // CONCLUSIONPAGE_HPP
