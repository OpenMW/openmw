#include "esmfile.hpp"

#include <QMimeData>
#include <QDataStream>

int ContentSelectorModel::EsmFile::sPropertyCount = 7;
QString ContentSelectorModel::EsmFile::sToolTip = QString("<b>Author:</b> %1<br/> \
                                              <b>Version:</b> %2<br/> \
                                              <br/><b>Description:</b><br/>%3<br/> \
                                              <br/><b>Dependencies: </b>%4<br/>");


ContentSelectorModel::EsmFile::EsmFile(QString fileName, ModelItem *parent)
                : ModelItem(parent), mFileName(fileName), mFormat(0)
{}

void ContentSelectorModel::EsmFile::setFileName(const QString &fileName)
{
    mFileName = fileName;
}

void ContentSelectorModel::EsmFile::setAuthor(const QString &author)
{
    mAuthor = author;
}

void ContentSelectorModel::EsmFile::setDate(const QDateTime &modified)
{
    mModified = modified;
}

void ContentSelectorModel::EsmFile::setFormat(int format)
{
    mFormat = format;
}

void ContentSelectorModel::EsmFile::setPath(const QString &path)
{
    mPath = path;
}

void ContentSelectorModel::EsmFile::setGameFiles(const QStringList &gamefiles)
{
    mGameFiles = gamefiles;
}

void ContentSelectorModel::EsmFile::setDescription(const QString &description)
{
    mDescription = description;
}

QByteArray ContentSelectorModel::EsmFile::encodedData() const
{
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    stream << mFileName << mAuthor << QString::number(mFormat)
           << mModified.toString() << mPath << mDescription
           << mGameFiles;

    return encodedData;
}

QVariant ContentSelectorModel::EsmFile::fileProperty(const FileProperty prop) const
{
    switch (prop)
    {
    case FileProperty_FileName:
        return mFileName;
        break;

    case FileProperty_Author:
        return mAuthor;
        break;

    case FileProperty_Format:
        return mFormat;
        break;

    case FileProperty_DateModified:
        return mModified.toString(Qt::ISODate);
        break;

    case FileProperty_Path:
        return mPath;
        break;

    case FileProperty_Description:
        return mDescription;
        break;

    case FileProperty_GameFile:
        return mGameFiles;
        break;

    default:
        break;
    }
    return QVariant();
}
void ContentSelectorModel::EsmFile::setFileProperty (const FileProperty prop, const QString &value)
{
    switch (prop)
    {
    case FileProperty_FileName:
        mFileName = value;
        break;

    case FileProperty_Author:
        mAuthor = value;
        break;

    case FileProperty_Format:
        mFormat = value.toInt();
        break;

    case FileProperty_DateModified:
        mModified = QDateTime::fromString(value);
        break;

    case FileProperty_Path:
        mPath = value;
        break;

    case FileProperty_Description:
        mDescription = value;
        break;

    case FileProperty_GameFile:
        mGameFiles << value;
        break;

    default:
        break;
    }
}
