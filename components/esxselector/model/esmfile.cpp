#include "esmfile.hpp"

#include <QMimeData>
#include <QDataStream>

int EsxModel::EsmFile::sPropertyCount = 7;
QString EsxModel::EsmFile::sToolTip = QString("<b>Author:</b> %1<br/> \
                                              <b>Version:</b> %2<br/> \
                                              <br/><b>Description:</b><br/>%3<br/> \
                                              <br/><b>Dependencies: </b>%4<br/>");


EsxModel::EsmFile::EsmFile(QString fileName, ModelItem *parent)
    : ModelItem(parent), mFileName(fileName), mVersion(0.0f)
{}
void EsxModel::EsmFile::setFileName(const QString &fileName)
{
    mFileName = fileName;
}

void EsxModel::EsmFile::setAuthor(const QString &author)
{
    mAuthor = author;
}

void EsxModel::EsmFile::setDate(const QDateTime &modified)
{
    mModified = modified;
}

void EsxModel::EsmFile::setVersion(float version)
{
    mVersion = version;
}

void EsxModel::EsmFile::setPath(const QString &path)
{
    mPath = path;
}

void EsxModel::EsmFile::setMasters(const QStringList &masters)
{
    mMasters = masters;
}

void EsxModel::EsmFile::setDescription(const QString &description)
{
    mDescription = description;
}

QByteArray EsxModel::EsmFile::encodedData() const
{
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    stream << mFileName << mAuthor << QString::number(mVersion)
           << mModified.toString() << mPath << mDescription
           << mMasters;

    return encodedData;
}

QVariant EsxModel::EsmFile::fileProperty(const FileProperty prop) const
{
    switch (prop)
    {
    case FileProperty_FileName:
        return mFileName;
        break;

    case FileProperty_Author:
        return mAuthor;
        break;

    case FileProperty_Version:
        return mVersion;
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

    case FileProperty_Master:
        return mMasters;
        break;

    default:
        break;
    }
    return QVariant();
}
void EsxModel::EsmFile::setFileProperty (const FileProperty prop, const QString &value)
{
    switch (prop)
    {
    case FileProperty_FileName:
        mFileName = value;
        break;

    case FileProperty_Author:
        mAuthor = value;
        break;

    case FileProperty_Version:
        mVersion = value.toFloat();
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

    case FileProperty_Master:
        mMasters << value;
        break;

    default:
        break;
    }
}
