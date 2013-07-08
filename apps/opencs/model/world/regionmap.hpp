#ifndef CSM_WOLRD_REGIONMAP_H
#define CSM_WOLRD_REGIONMAP_H

#include <map>
#include <string>

#include <QAbstractTableModel>

namespace CSMWorld
{
    /// \brief Model for the region map
    ///
    /// This class does not holds any record data (other than for the purpose of buffering).
    class RegionMap : public QAbstractTableModel
    {
            std::map<std::pair<int, int>, std::string> mMap; ///< cell index, region
            std::pair<int, int> mMin; ///< inclusive
            std::pair<int, int> mMax; ///< exclusive
            std::map<std::string, unsigned int> mColours; ///< region ID, colour (RGBA)

            std::pair<int, int> getIndex (const QModelIndex& index) const;
            ///< Translates a Qt model index into a cell index (which can contain negative components)

        public:

            RegionMap();

            virtual int rowCount (const QModelIndex& parent = QModelIndex()) const;

            virtual int columnCount (const QModelIndex& parent = QModelIndex()) const;

            virtual QVariant data (const QModelIndex& index, int role = Qt::DisplayRole) const;

            virtual Qt::ItemFlags flags (const QModelIndex& index) const;
    };
}

#endif
