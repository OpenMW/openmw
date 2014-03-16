#ifndef UNSHIELDWORKER_HPP
#define UNSHIELDWORKER_HPP

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QReadWriteLock>

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
        Q_ENUMS(Wizard::Component)

    public:
        UnshieldWorker(QObject *parent = 0);
        ~UnshieldWorker();

        void setInstallComponent(Wizard::Component component, bool install);

//        void setComponentPath(Wizard::Component component, const QString &path);
        void setDiskPath(const QString &path);

        void setPath(const QString &path);
        void setIniPath(const QString &path);

        QString getPath();
        QString getIniPath();

        void setIniCodec(QTextCodec *codec);

        void setupSettings();

    private:

        void writeSettings();

        bool getInstallComponent(Component component);

        //QString getComponentPath(Component component);
        QString getDiskPath();

        void setComponentDone(Component component, bool done = true);
        bool getComponentDone(Component component);

        bool removeDirectory(const QString &dirName);

        bool copyFile(const QString &source, const QString &destination, bool keepSource = true);
        bool copyDirectory(const QString &source, const QString &destination, bool keepSource = true);

        bool moveFile(const QString &source, const QString &destination);
        bool moveDirectory(const QString &source, const QString &destination);

        bool extractCab(const QString &cabFile, const QString &destination);
        bool extractFile(Unshield *unshield, const QString &destination, const QString &prefix, int index, int counter);
        bool findInCab(const QString &cabFile, const QString &fileName);

        QString findFile(const QString &fileName, const QString &path, int depth = 0);
        QStringList findFiles(const QString &fileName, const QString &path, int depth = 0);

        bool installFile(const QString &fileName, const QString &path);
        bool installDirectory(const QString &dirName, const QString &path, bool recursive = true);

        bool installComponent(Component component, const QString &path);
        bool setupComponent(Component component);

        bool mInstallMorrowind;
        bool mInstallTribunal;
        bool mInstallBloodmoon;

        bool mMorrowindDone;
        bool mTribunalDone;
        bool mBloodmoonDone;

        QString mPath;
        QString mIniPath;
        QString mDiskPath;

        IniSettings mIniSettings;

        QTextCodec *mIniCodec;

        QWaitCondition mWait;
        QMutex mMutex;

        QReadWriteLock mLock;

    public slots:
        void extract();

    signals:
        void finished();
        void requestFileDialog(Wizard::Component component);

        void textChanged(const QString &text);
        void logTextChanged(const QString &text);

        void error(const QString &text, const QString &details);
        void progressChanged(int progress);


    };
}

#endif // UNSHIELDWORKER_HPP
