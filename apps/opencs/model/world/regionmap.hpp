#ifndef CSM_WOLRD_REGIONMAP_H
#define CSM_WOLRD_REGIONMAP_H

#include <map>
#include <string>

#include <QAbstractTableModel>

namespace CSMWorld
{
    class Data;

    /// \brief Model for the region map
    ///
    /// This class does not holds any record data (other than for the purpose of buffering).
    class RegionMap : public QAbstractTableModel
    {
            Q_OBJECT

        public:

            typedef std::pair<int, int> CellIndex;

        private:

            struct CellDescription
            {
                bool mDeleted;
                std::string mRegion;

                CellDescription();
            };

            std::map<CellIndex, CellDescription> mMap;
            CellIndex mMin; ///< inclusive
            CellIndex mMax; ///< exclusive
            std::map<std::string, unsigned int> mColours; ///< region ID, colour (RGBA)

            CellIndex getIndex (const QModelIndex& index) const;
            ///< Translates a Qt model index into a cell index (which can contain negative components)

            void buildRegions (Data& data);

            void buildMap (Data& data);

        public:

            RegionMap (Data& data);

            virtual int rowCount (const QModelIndex& parent = QModelIndex()) const;

            virtual int columnCount (const QModelIndex& parent = QModelIndex()) const;

            virtual QVariant data (const QModelIndex& index, int role = Qt::DisplayRole) const;

            virtual Qt::ItemFlags flags (const QModelIndex& index) const;

        private slots:

            void regionsAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            void regionsInserted (const QModelIndex& parent, int start, int end);

            void regionsChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void cellsAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            void cellsInserted (const QModelIndex& parent, int start, int end);

            void cellsChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);
    };
}

#endif
