#ifndef UNSHIELDTHREAD_HPP
#define UNSHIELDTHREAD_HPP

#include <QThread>

#include <libunshield.h>

namespace Wizard
{

    class UnshieldThread : public QThread
    {
        Q_OBJECT
    public:
        explicit UnshieldThread(QObject *parent = 0);

        void setInstallMorrowind(bool install);
        void setInstallTribunal(bool install);
        void setInstallBloodmoon(bool install);

        void setPath(const QString &path);

    private:
        void extract();

        void extractCab(const QString &cabFile,
                        const QString &outputDir, bool extractIni);
        //void extractFile(Unshield *unshield,
        //                 )

        bool mInstallMorrowind;
        bool mInstallTribunal;
        bool mInstallBloodmoon;

        QString mPath;

    protected:
        virtual void run();

    signals:
        void textChanged(const QString &text);

    };

}

#endif // UNSHIELDTHREAD_HPP
