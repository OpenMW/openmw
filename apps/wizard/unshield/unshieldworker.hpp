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

        void setComponentPath(Wizard::Component component, const QString &path);

        void setPath(const QString &path);
        void setIniPath(const QString &path);

        QString getPath();
        QString getIniPath();

        void setIniCodec(QTextCodec *codec);

    private:

        bool getInstallComponent(Component component);

        QString getComponentPath(Component component);

        void setComponentDone(Component component, bool done = true);
        bool getComponentDone(Component component);

        bool removeDirectory(const QString &dirName);

        bool copyFile(const QString &source, const QString &destination, bool keepSource = true);
        bool copyDirectory(const QString &source, const QString &destination, bool keepSource = true);

        bool moveFile(const QString &source, const QString &destination);
        bool moveDirectory(const QString &source, const QString &destination);

        void setupSettings();

        bool extractCab(const QString &cabFile, const QString &outputDir);
        bool extractFile(Unshield *unshield, const QString &outputDir, const QString &prefix, int index, int counter);
        bool findFile(const QString &cabFile, const QString &fileName);

        void installDirectories(const QString &source);

        bool installMorrowind();
        bool installTribunal();
        bool installBloodmoon();

        bool installComponent(Component component);
        void setupAddon(Component component);

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
