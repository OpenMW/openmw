#include "esmfile.hpp"

EsmFile::EsmFile(QString fileName, ModelItem *parent)
    : ModelItem(parent)
{
     mFileName = fileName;
     mSize = 0;
     mVersion = 0.0f;
}

void EsmFile::setFileName(const QString &fileName)
{
    mFileName = fileName;
}

void EsmFile::setAuthor(const QString &author)
{
    mAuthor = author;
}

void EsmFile::setSize(const int size)
{
    mSize = size;
}

void EsmFile::setDates(const QDateTime &modified, const QDateTime &accessed)
{
    mModified = modified;
    mAccessed = accessed;
}

void EsmFile::setVersion(float version)
{
    mVersion = version;
}

void EsmFile::setPath(const QString &path)
{
    mPath = path;
}

void EsmFile::setMasters(const QStringList &masters)
{
    mMasters = masters;
}

void EsmFile::setDescription(const QString &description)
{
    mDescription = description;
}
