#ifndef CSMSETTINGS_CONNECTOR_HPP
#define CSMSETTINGS_CONNECTOR_HPP

#include <QObject>
#include <QList>
#include <QMap>
#include <QStringList>

#include "../../view/settings/support.hpp"

namespace CSVSettings {
    class View;
}

namespace CSMSettings {

    class Connector : public QObject
    {
        Q_OBJECT

        CSVSettings::View *mMasterView;

        //map using the view pointer as a key to it's index value
        QList <CSVSettings::View *> mSlaveViews;

        //list of proxy values for each master value.
        //value list order is indexed to the master value index.
        QMap < QString, QList <QStringList> > mProxyListMap;

    public:
        explicit Connector(CSVSettings::View *master,
                                QObject *parent = 0);

        void setMasterView (CSVSettings::View *view);
        void addSlaveView (CSVSettings::View *view,
                                     QList <QStringList> &masterProxyValues);

    private:

        bool proxyListsMatch (const QList <QStringList> &list1,
                              const QList <QStringList> &list2) const;

        bool stringListsMatch (const QStringList &list1,
                               const QStringList &list2) const;

        QList <QStringList> getSlaveViewValues() const;

    public slots:

        void slotUpdateSlaves() const;
        void slotUpdateMaster() const;
    };
}

#endif // CSMSETTINGS_CONNECTOR_HPP
