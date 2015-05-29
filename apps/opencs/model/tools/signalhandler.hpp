#ifndef CSM_TOOLS_SIGNALHANDLER_H
#define CSM_TOOLS_SIGNALHANDLER_H

#include <QObject>

namespace CSMTools
{
    class SignalHandler : public QObject
    {
        Q_OBJECT

            bool mExtraCheck;

        public:

            SignalHandler (bool extraCheck);

            bool extraCheck ();

        public slots:

            void updateUserSetting (const QString &name, const QStringList &list);
    };
}

#endif
