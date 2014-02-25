#ifndef PROCESSINVOKER_HPP
#define PROCESSINVOKER_HPP

#include <QStringList>
#include <QString>

namespace Process
{
    class ProcessInvoker : public QObject
    {
        Q_OBJECT

        ProcessInvoker();
        ~ProcessInvoker();

    public:

        inline static bool startProcess(const QString &name, bool detached = false) { return startProcess(name, QStringList(), detached); }
        bool static startProcess(const QString &name, const QStringList &arguments, bool detached = false);
    };
}

#endif // PROCESSINVOKER_HPP
