#include "cellnameloader.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/files/qtconversion.hpp>

QSet<QString> CellNameLoader::getCellNames(const QStringList& contentPaths)
{
    QSet<QString> cellNames;
    ESM::ESMReader esmReader;

    // Loop through all content files
    for (const QString& contentPath : contentPaths)
    {
        if (contentPath.endsWith(".omwscripts", Qt::CaseInsensitive))
            continue;
        try
        {
            esmReader.open(Files::pathFromQString(contentPath));

            // Loop through all records
            while (esmReader.hasMoreRecs())
            {
                ESM::NAME recordName = esmReader.getRecName();
                esmReader.getRecHeader();

                if (isCellRecord(recordName))
                {
                    QString cellName = getCellName(esmReader);
                    if (!cellName.isEmpty())
                    {
                        cellNames.insert(cellName);
                    }
                }

                // Stop loading content for this record and continue to the next
                esmReader.skipRecord();
            }
        }
        catch (const std::exception& e)
        {
            Log(Debug::Error) << "Failed to get cell names from " << contentPath.toStdString() << ": " << e.what();
        }
    }

    return cellNames;
}

bool CellNameLoader::isCellRecord(ESM::NAME& recordName)
{
    return recordName.toInt() == ESM::REC_CELL;
}

QString CellNameLoader::getCellName(ESM::ESMReader& esmReader)
{
    ESM::Cell cell;
    bool isDeleted = false;
    cell.loadNameAndData(esmReader, isDeleted);

    return QString::fromStdString(cell.mName);
}
