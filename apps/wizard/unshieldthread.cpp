#include "unshieldthread.hpp"

#include <QDebug>
#include <QStringList>

Wizard::UnshieldThread::UnshieldThread(QObject *parent) :
    QThread(parent)
{
    unshield_set_log_level(0);

    mInstallMorrowind = false;
    mInstallTribunal = false;
    mInstallBloodmoon = false;

}

void Wizard::UnshieldThread::setInstallMorrowind(bool install)
{
    mInstallMorrowind = install;
}

void Wizard::UnshieldThread::setInstallTribunal(bool install)
{
    mInstallTribunal = install;
}

void Wizard::UnshieldThread::setInstallBloodmoon(bool install)
{
    mInstallBloodmoon = install;
}

void Wizard::UnshieldThread::setPath(const QString &path)
{
    mPath = path;
}

void Wizard::UnshieldThread::extract()
{
    emit textChanged(QLatin1String("Starting installation"));
    emit textChanged(QLatin1String("Installation target: ") + mPath);

    QStringList components;
    if (mInstallMorrowind)
        components << QLatin1String("Morrowind");

    if (mInstallTribunal)
        components << QLatin1String("Tribunal");

    if (mInstallBloodmoon)
        components << QLatin1String("Bloodmoon");

    emit textChanged(QLatin1String("Components: ") + components.join(QLatin1String(", ")));

}

void Wizard::UnshieldThread::run()
{
    qDebug() << "From worker thread: " << currentThreadId();

    extract();
}
