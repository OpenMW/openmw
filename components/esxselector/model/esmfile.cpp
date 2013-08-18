#include "esmfile.hpp"

EsxModel::EsmFile::EsmFile(QString fileName, ModelItem *parent)
    : ModelItem(parent)
{
     mFileName = fileName;
     mSize = 0;
     mVersion = 0.0f;
}

void EsxModel::EsmFile::setFileName(const QString &fileName)
{
    mFileName = fileName;
}

void EsxModel::EsmFile::setAuthor(const QString &author)
{
    mAuthor = author;
}

void EsxModel::EsmFile::setSize(const int size)
{
    mSize = size;
}

void EsxModel::EsmFile::setDates(const QDateTime &modified, const QDateTime &accessed)
{
    mModified = modified;
    mAccessed = accessed;
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
