#ifndef UNSHIELDTHREAD_HPP
#define UNSHIELDTHREAD_HPP

#include <QThread>

#include <libunshield.h>

#include "inisettings.hpp"

class QTextCodec;

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
        void setIniPath(const QString &path);

        void setIniCodec(QTextCodec *codec);

    private:

        void setupSettings();
        void extract();

        void extractCab(const QString &cabFile, const QString &outputDir);
        bool extractFile(Unshield *unshield, const QString &outputDir, const QString &prefix, int index, int counter);


        bool mInstallMorrowind;
        bool mInstallTribunal;
        bool mInstallBloodmoon;

        QString mPath;
        QString mIniPath;

        IniSettings mIniSettings;

        QTextCodec *mIniCodec;

    protected:
        virtual void run();

    signals:
        void textChanged(const QString &text);
        void progressChanged(int progress);

    };

}

#endif // UNSHIELDTHREAD_HPP
