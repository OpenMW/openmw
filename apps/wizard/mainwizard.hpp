#ifndef MAINWIZARD_HPP
#define MAINWIZARD_HPP

#include <QWizard>
#include <QMap>

namespace Wizard
{

    class MainWizard : public QWizard
    {
        Q_OBJECT

    public:
        struct Installation {
            bool hasMorrowind;
            bool hasTribunal;
            bool hasBloodmoon;
        };

        enum {
            Page_Intro,
            Page_MethodSelection,
            Page_LanguageSelection,
            Page_ExistingInstallation,
            Page_InstallationTarget,
            Page_ComponentSelection,
            Page_Installation,
            Page_Import,
            Page_Conclusion
        };

        MainWizard(QWidget *parent = 0);

        static bool findFiles(const QString &name, const QString &path);
        QMap<QString, Installation*> mInstallations;

    private:
        void setupInstallations();
        void setupPages();


    };

}

#endif // MAINWIZARD_HPP
