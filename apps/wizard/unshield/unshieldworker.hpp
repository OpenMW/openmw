#ifndef UNSHIELDWORKER_HPP
#define UNSHIELDWORKER_HPP

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

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

        bool getInstallMorrowind();
        bool getInstallTribunal();
        bool getInstallBloodmoon();

        void setMorrowindPath(const QString &path);
        void setTribunalPath(const QString &path);
        void setBloodmoonPath(const QString &path);

        QString getMorrowindPath();
        QString getTribunalPath();
        QString getBloodmoonPath();

        void setPath(const QString &path);
        void setIniPath(const QString &path);

        void setIniCodec(QTextCodec *codec);

    private:

//        void setMorrowindDone(bool done);
//        void setTribunalDone(bool done);
//        void setBloodmoonDone(bool done);

//        bool getMorrowindDone();
//        bool getTribunalDone();
//        bool getBloodmoonDone();

        bool removeDirectory(const QString &dirName);

        bool copyFile(const QString &source, const QString &destination, bool keepSource = true);
        bool copyDirectory(const QString &source, const QString &destination, bool keepSource = true);

        bool moveFile(const QString &source, const QString &destination);
        bool moveDirectory(const QString &source, const QString &destination);

        void setupSettings();

        void extractCab(const QString &cabFile, const QString &outputDir);
        bool extractFile(Unshield *unshield, const QString &outputDir, const QString &prefix, int index, int counter);
        bool findFile(const QString &cabFile, const QString &fileName);

        void installDirectories(const QString &source);

        bool installMorrowind();
        bool installTribunal();
        bool installBloodmoon();

        bool mInstallMorrowind;
        bool mInstallTribunal;
        bool mInstallBloodmoon;

        bool mMorrowindDone;
        bool mTribunalDone;
        bool mBloodmoonDone;

        QString mMorrowindPath;
        QString mTribunalPath;
        QString mBloodmoonPath;

        QString mPath;
        QString mIniPath;

        IniSettings mIniSettings;

        QTextCodec *mIniCodec;

        QWaitCondition mWait;
        QMutex mMutex;

    public slots:
        void extract();

    signals:
        void finished();
        void requestFileDialog(const QString &component);

        void textChanged(const QString &text);
        void logTextChanged(const QString &text);

        void error(const QString &text);
        void progressChanged(int progress);


    };
}

#endif // UNSHIELDWORKER_HPP
