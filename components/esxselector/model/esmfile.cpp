#include "esmfile.hpp"

#include <QMimeData>
#include <QDataStream>

int EsxModel::EsmFile::sPropertyCount = 7;

EsxModel::EsmFile::EsmFile(QString fileName, ModelItem *parent)
    : ModelItem(parent), mFileName(fileName), mVersion(0.0f)
{}
/*
EsxModel::EsmFile::EsmFile(const EsmFile &file)
    : ModelItem(file.parent()), mFileName(file.mFileName), mSize(file.mSize),
      mVersion(file.mVersion), mAuthor(file.mAuthor), mModified(file.mModified),
      mAccessed(file.mAccessed), mPath(file.mPath), mMasters(file.mMasters),
      mDescription(file.mDescription)
{}

*/
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

void EsxModel::EsmFile::setProperty (const EsmFileProperty prop, const QString &value)
{
    switch (prop)
    {
    case Property_FileName:
        mFileName = value;
        break;

    case Property_Author:
        mAuthor = value;
        break;

    case Property_Version:
        mVersion = value.toFloat();
        break;

    case Property_DateModified:
        mModified = QDateTime::fromString(value);
        break;

    case Property_Path:
        mPath = value;
        break;

    case Property_Description:
        mDescription = value;
        break;

    case Property_Master:
        mMasters << value;
        break;

    default:
        break;
    }
}
