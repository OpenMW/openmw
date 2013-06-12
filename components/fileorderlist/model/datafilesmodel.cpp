#include <QTextDecoder>
#include <QTextCodec>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

#include <stdexcept>

#include <components/esm/esmreader.hpp>

#include "esm/esmfile.hpp"

#include "datafilesmodel.hpp"

DataFilesModel::DataFilesModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    mEncoding = QString("win1252");
}

DataFilesModel::~DataFilesModel()
{
}

void DataFilesModel::setEncoding(const QString &encoding)
{
    mEncoding = encoding;
}

void DataFilesModel::setCheckState(const QModelIndex &index, Qt::CheckState state)
{
    setData(index, state, Qt::CheckStateRole);
}

Qt::CheckState DataFilesModel::checkState(const QModelIndex &index)
{
    EsmFile *file = item(index.row());
    return mCheckStates[file->fileName()];
}

int DataFilesModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 9;
}

int DataFilesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : mFiles.count();
}


bool DataFilesModel::moveRow(int oldrow, int row, const QModelIndex &parent)
{
    if (oldrow < 0 || row < 0 || oldrow == row)
        return false;

    emit layoutAboutToBeChanged();
    //emit beginMoveRows(parent, oldrow, oldrow, parent, row);
    mFiles.swap(oldrow, row);
    //emit endInsertRows();
    emit layoutChanged();

    return true;
}

QVariant DataFilesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    EsmFile *file = item(index.row());

    if (!file)
        return QVariant();

    const int column = index.column();

    switch (role) {
    case Qt::DisplayRole: {

        switch (column) {
        case 0:
            return file->fileName();
        case 1:
            return file->author();
        case 2:
            return QString("%1 kB").arg(int((file->size() + 1023) / 1024));
        case 3:
            //return file->modified().toString(Qt::TextDate);
            return file->modified().toString(Qt::ISODate);
        case 4:
            return file->accessed().toString(Qt::TextDate);
        case 5:
            return file->version();
        case 6:
            return file->path();
        case 7:
            return file->masters().join(", ");
        case 8:
            return file->description();
        }
    }

    case Qt::TextAlignmentRole: {
        switch (column) {
        case 0:
        case 1:
            return Qt::AlignLeft + Qt::AlignVCenter;
        case 2:
        case 3:
        case 4:
        case 5:
            return Qt::AlignRight + Qt::AlignVCenter;
        default:
            return Qt::AlignLeft + Qt::AlignVCenter;
        }
    }

    case Qt::CheckStateRole: {
        if (column != 0)
            return QVariant();
        return mCheckStates[file->fileName()];
    }
    case Qt::ToolTipRole:
    {
        if (column != 0)
            return QVariant();

        if (file->version() == 0.0f)
            return QVariant(); // Data not set

        QString tooltip =
                QString("<b>Author:</b> %1<br/> \
                        <b>Version:</b> %2<br/> \
                        <br/><b>Description:</b><br/>%3<br/> \
                        <br/><b>Dependencies: </b>%4<br/>")
                        .arg(file->author())
                        .arg(QString::number(file->version()))
                        .arg(file->description())
                        .arg(file->masters().join(", "));


        return tooltip;

    }
    default:
        return QVariant();
    }

}

Qt::ItemFlags DataFilesModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    EsmFile *file = item(index.row());

    if (!file)
        return Qt::NoItemFlags;

    if (canBeChecked(file)) {
        if (index.column() == 0) {
            return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        } else {
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        }
    } else {
        if (index.column() == 0) {
            return Qt::ItemIsUserCheckable | Qt::ItemIsSelectable;
        } else {
            return Qt::NoItemFlags | Qt::ItemIsSelectable;
        }
    }

}

QVariant DataFilesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return tr("Name");
        case 1: return tr("Author");
        case 2: return tr("Size");
        case 3: return tr("Modified");
        case 4: return tr("Accessed");
        case 5: return tr("Version");
        case 6: return tr("Path");
        case 7: return tr("Masters");
        case 8: return tr("Description");
        }
    } else {
        // Show row numbers
        return ++section;
    }

    return QVariant();
}

bool DataFilesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
            return false;

    if (role == Qt::CheckStateRole) {
        QString name = item(index.row())->fileName();
        mCheckStates[name] = static_cast<Qt::CheckState>(value.toInt());

        // Force a redraw of the view since unchecking one item can affect another
        QModelIndex firstIndex = indexFromItem(mFiles.first());
        QModelIndex lastIndex = indexFromItem(mFiles.last());

        emit dataChanged(firstIndex, lastIndex);
        emit checkedItemsChanged(checkedItems());
        return true;
    }

    return false;
}

bool lessThanEsmFile(const EsmFile *e1, const EsmFile *e2)
{
    //Masters first then alphabetically
    if (e1->fileName().endsWith(".esm") && !e2->fileName().endsWith(".esm"))
        return true;
    if (!e1->fileName().endsWith(".esm") && e2->fileName().endsWith(".esm"))
        return false;

    return e1->fileName().toLower() < e2->fileName().toLower();
}

bool lessThanDate(const EsmFile *e1, const EsmFile *e2)
{
    if (e1->modified().toString(Qt::ISODate) < e2->modified().toString(Qt::ISODate)) {
        return true;
    } else {
        return false;
    }
}

void DataFilesModel::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();

    if (column == 3) {
        qSort(mFiles.begin(), mFiles.end(), lessThanDate);
    } else {
        qSort(mFiles.begin(), mFiles.end(), lessThanEsmFile);
    }

    emit layoutChanged();
}

void DataFilesModel::addFile(EsmFile *file)
{
    emit beginInsertRows(QModelIndex(), mFiles.count(), mFiles.count());
    mFiles.append(file);
    emit endInsertRows();
}

void DataFilesModel::addFiles(const QString &path)
{
    QDir dir(path);
    QStringList filters;
    filters << "*.esp" << "*.esm";
    dir.setNameFilters(filters);

    // Create a decoder for non-latin characters in esx metadata
    QTextCodec *codec;

    if (mEncoding == QLatin1String("win1252")) {
        codec = QTextCodec::codecForName("windows-1252");
    } else if (mEncoding == QLatin1String("win1251")) {
        codec = QTextCodec::codecForName("windows-1251");
    } else if (mEncoding == QLatin1String("win1250")) {
        codec = QTextCodec::codecForName("windows-1250");
    } else {
        return; // This should never happen;
    }

    QTextDecoder *decoder = codec->makeDecoder();

    foreach (const QString &path, dir.entryList()) {
        QFileInfo info(dir.absoluteFilePath(path));
        EsmFile *file = new EsmFile(path);

        try {
            ESM::ESMReader fileReader;
            ToUTF8::Utf8Encoder encoder(ToUTF8::calculateEncoding(mEncoding.toStdString()));
            fileReader.setEncoder(&encoder);
            fileReader.open(dir.absoluteFilePath(path).toStdString());

            std::vector<ESM::Header::MasterData> mlist = fileReader.getMasters();

            QStringList masters;

            for (unsigned int i = 0; i < mlist.size(); ++i) {
                QString master = QString::fromStdString(mlist[i].name);
                masters.append(master);
            }

            file->setAuthor(decoder->toUnicode(fileReader.getAuthor().c_str()));
            file->setSize(info.size());
            file->setDates(info.lastModified(), info.lastRead());
            file->setVersion(fileReader.getFVer());
            file->setPath(info.absoluteFilePath());
            file->setMasters(masters);
            file->setDescription(decoder->toUnicode(fileReader.getDesc().c_str()));


            // Put the file in the table
            if (findItem(path) == 0)
                addFile(file);
        } catch(std::runtime_error &e) {
            // An error occurred while reading the .esp
            qWarning() << "Error reading esp: " << e.what();
            continue;
        }

    }

    delete decoder;
}

QModelIndex DataFilesModel::indexFromItem(EsmFile *item) const
{
    if (item)
        return createIndex(mFiles.indexOf(item), 0);

    return QModelIndex();
}

EsmFile* DataFilesModel::findItem(const QString &name)
{
    QList<EsmFile *>::ConstIterator it;
    QList<EsmFile *>::ConstIterator itEnd = mFiles.constEnd();

    int i = 0;
    for (it = mFiles.constBegin(); it != itEnd; ++it) {
        EsmFile *file = item(i);
        ++i;

        if (name == file->fileName())
            return file;
    }

    // Not found
    return 0;
}

EsmFile* DataFilesModel::item(int row) const
{
    if (row >= 0 && row < mFiles.count())
        return mFiles.at(row);
    else
        return 0;
}

QStringList DataFilesModel::checkedItems()
{
    QStringList list;

    QList<EsmFile *>::ConstIterator it;
    QList<EsmFile *>::ConstIterator itEnd = mFiles.constEnd();

    int i = 0;
    for (it = mFiles.constBegin(); it != itEnd; ++it) {
        EsmFile *file = item(i);
        ++i;

        QString name = file->fileName();

        // Only add the items that are in the checked list and available
        if (mCheckStates[name] == Qt::Checked && canBeChecked(file))
            list << name;
    }

    return list;
}

QStringList DataFilesModel::checkedItemsPaths()
{
    QStringList list;

    QList<EsmFile *>::ConstIterator it;
    QList<EsmFile *>::ConstIterator itEnd = mFiles.constEnd();

    int i = 0;
    for (it = mFiles.constBegin(); it != itEnd; ++it) {
        EsmFile *file = item(i);
        ++i;

        if (mCheckStates[file->fileName()] == Qt::Checked && canBeChecked(file))
            list << file->path();
    }

    return list;
}

void DataFilesModel::uncheckAll()
{
    emit layoutAboutToBeChanged();
    mCheckStates.clear();
    emit layoutChanged();
}

QStringList DataFilesModel::uncheckedItems()
{
    QStringList list;
    QStringList checked = checkedItems();

    QList<EsmFile *>::ConstIterator it;
    QList<EsmFile *>::ConstIterator itEnd = mFiles.constEnd();

    int i = 0;
    for (it = mFiles.constBegin(); it != itEnd; ++it) {
        EsmFile *file = item(i);
        ++i;

        // Add the items that are not in the checked list
        if (!checked.contains(file->fileName()))
            list << file->fileName();
    }

    return list;
}

bool DataFilesModel::canBeChecked(EsmFile *file) const
{
    //element can be checked if all its dependencies are
    bool canBeChecked = true;
    foreach (const QString &master, file->masters())
    {
        if (!mCheckStates.contains(master) || mCheckStates[master] != Qt::Checked)
        {
            canBeChecked = false;
            break;
        }
    }
    return canBeChecked;
}
