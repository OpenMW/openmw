#ifndef UNSHIELDWORKER_HPP
#define UNSHIELDWORKER_HPP

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QReadWriteLock>
#include <QStringList>

#include <libunshield.h>

#include "../inisettings.hpp"


namespace Wizard
{
    enum Component {
        Component_Morrowind,
        Component_Tribunal,
        Component_Bloodmoon
    };

    class UnshieldWorker : public QObject
    {
        Q_OBJECT

    public:
        UnshieldWorker(QObject *parent = nullptr);
        ~UnshieldWorker() override;

        void stopWorker();

        void setInstallComponent(Wizard::Component component, bool install);

        void setDiskPath(const QString &path);

        void setPath(const QString &path);
        void setIniPath(const QString &path);

        QString getPath();
        QString getIniPath();

        void setIniCodec(QTextCodec *codec);

        bool setupSettings();

    private:

        bool writeSettings();

        bool getInstallComponent(Component component);

        QString getDiskPath();

        void setComponentDone(Component component, bool done = true);
        bool getComponentDone(Component component);

        bool removeDirectory(const QString &dirName);

        bool copyFile(const QString &source, const QString &destination, bool keepSource = true);
        bool copyDirectory(const QString &source, const QString &destination, bool keepSource = true);

        bool extractCab(const QString &cabFile, const QString &destination);
        bool extractFile(Unshield *unshield, const QString &destination, const QString &prefix, int index, int counter);

        bool findInCab(const QString &fileName, const QString &cabFile);

        QString findFile(const QString &fileName, const QString &path);

        QStringList findFiles(const QString &fileName, const QString &path, int depth = 0, bool recursive = true,
                              bool directories = false, Qt::MatchFlags flags = Qt::MatchExactly);

        QStringList findDirectories(const QString &dirName, const QString &path, bool recursive = true);

        bool installFile(const QString &fileName, const QString &path, Qt::MatchFlags flags = Qt::MatchExactly,
                         bool keepSource = false);

        bool installFiles(const QString &fileName, const QString &path, Qt::MatchFlags flags = Qt::MatchExactly,
                          bool keepSource = false, bool single = false);

        bool installDirectories(const QString &dirName, const QString &path,
                                bool recursive = true, bool keepSource = false);

        bool installComponent(Component component, const QString &path);
        bool setupComponent(Component component);

        bool mInstallMorrowind;
        bool mInstallTribunal;
        bool mInstallBloodmoon;

        bool mMorrowindDone;
        bool mTribunalDone;
        bool mBloodmoonDone;

        bool mStopped;

        QString mPath;
        QString mIniPath;
        QString mDiskPath;

        IniSettings mIniSettings;

        QTextCodec *mIniCodec;

        QWaitCondition mWait;

        QReadWriteLock mLock;

    public slots:
        void extract();

    signals:
        void finished();
        void requestFileDialog(Wizard::Component component);

        void textChanged(const QString &text);

        void error(const QString &text, const QString &details);
        void progressChanged(int progress);

    };
}

#endif // UNSHIELDWORKER_HPP
