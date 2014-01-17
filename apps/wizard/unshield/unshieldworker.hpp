#ifndef UNSHIELDWORKER_HPP
#define UNSHIELDWORKER_HPP

#include <QObject>
#include <QThread>

#include <libunshield.h>

#include "../inisettings.hpp"

namespace Wizard
{
    class UnshieldWorker : public QObject
    {
        Q_OBJECT

    public:
        UnshieldWorker(QObject *parent = 0);
        ~UnshieldWorker();

        void setInstallMorrowind(bool install);
        void setInstallTribunal(bool install);
        void setInstallBloodmoon(bool install);

        void setPath(const QString &path);
        void setIniPath(const QString &path);

        void setIniCodec(QTextCodec *codec);

    private:

        bool removeDirectory(const QString &dirName);

        bool copyFile(const QString &source, const QString &destination, bool keepSource = true);
        bool copyDirectory(const QString &source, const QString &destination, bool keepSource = true);

        bool moveFile(const QString &source, const QString &destination);
        bool moveDirectory(const QString &source, const QString &destination);

        void setupSettings();

        void extractCab(const QString &cabFile, const QString &outputDir);
        bool extractFile(Unshield *unshield, const QString &outputDir, const QString &prefix, int index, int counter);

        void installDirectories(const QString &source);


        bool mInstallMorrowind;
        bool mInstallTribunal;
        bool mInstallBloodmoon;

        bool mMorrowindDone;
        bool mTribunalDone;
        bool mBloodmoonDone;

        QString mPath;
        QString mIniPath;

        IniSettings mIniSettings;

        QTextCodec *mIniCodec;


    public slots:
        void extract();

    signals:
        void finished();
        void textChanged(const QString &text);
        void logTextChanged(const QString &text);

        void error(const QString &text);
        void progressChanged(int progress);


    };
}

#endif // UNSHIELDWORKER_HPP
