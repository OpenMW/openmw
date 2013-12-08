#ifndef MAINWIZARD_HPP
#define MAINWIZARD_HPP

#include <QWizard>

namespace Wizard
{

    class MainWizard : public QWizard
    {
        Q_OBJECT

    public:
        enum {
            Page_Intro,
            Page_MethodSelection,
            Page_ExistingInstallation,
            Page_InstallationTarget,
            Page_ComponentSelection,
            Page_Installation,
            Page_Import,
            Page_Conclusion
        };

        MainWizard(QWidget *parent = 0);

    private:
        void setupPages();
    };

}

#endif // MAINWIZARD_HPP
