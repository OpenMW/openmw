#ifndef CSMSETTINGS_CONNECTOR_HPP
#define CSMSETTINGS_CONNECTOR_HPP

#include <QObject>
#include <QList>
#include <QMap>
#include <QStringList>

#include "support.hpp"

namespace CSVSettings {
    class View;
}

namespace CSMSettings {

    class Connector : public QObject
    {
        Q_OBJECT

        CSVSettings::View *mMasterView;

        ///map using the view pointer as a key to it's index value
        QList <CSVSettings::View *> mSlaveViews;

        ///list of proxy values for each master value.
        ///value list order is indexed to the master value index.
        QMap < QString, QList <QStringList> > mProxyListMap;

    public:
        explicit Connector(CSVSettings::View *master,
                                QObject *parent = 0);

        ///Set the view which acts as a proxy for other setting views
        void setMasterView (CSVSettings::View *view);

        ///Add a view to be updated / update to the master
        void addSlaveView (CSVSettings::View *view,
                                     QList <QStringList> &masterProxyValues);

    private:

        ///loosely matches lists of proxy values across registered slaves
        ///against a proxy value list for a given master value
        bool proxyListsMatch (const QList <QStringList> &list1,
                              const QList <QStringList> &list2) const;

        ///loosely matches two string lists
        bool stringListsMatch (const QStringList &list1,
                               const QStringList &list2) const;

        ///retrieves current values of registered slave views
        QList <QStringList> getSlaveViewValues() const;

    public slots:

        ///updates slave views with proxy values associated with current
        ///master value
        void slotUpdateSlaves() const;

        ///updates master value associated with the currently selected
        ///slave values, if applicable.
        void slotUpdateMaster() const;
    };
}

#endif // CSMSETTINGS_CONNECTOR_HPP
