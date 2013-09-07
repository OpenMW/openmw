#ifndef ESMFILE_HPP
#define ESMFILE_HPP

#include <QDateTime>
#include <QStringList>

#include "modelitem.hpp"

namespace EsxModel
{
    class EsmFile : public ModelItem
    {
        Q_OBJECT
        Q_PROPERTY(QString filename READ fileName)

    public:

        EsmFile(QString fileName = QString(), ModelItem *parent = 0);
     //   EsmFile(const EsmFile &);

        ~EsmFile()
        {}

        void setFileName(const QString &fileName);
        void setAuthor(const QString &author);
        void setSize(const int size);
        void setDates(const QDateTime &modified, const QDateTime &accessed);
        void setVersion(const float version);
        void setPath(const QString &path);
        void setMasters(const QStringList &masters);
        void setDescription(const QString &description);

        inline QString fileName() const { return mFileName; }
        inline QString author() const { return mAuthor; }
        inline int size() const { return mSize; }
        inline QDateTime modified() const { return mModified; }
        inline QDateTime accessed() const { return mAccessed; }
        inline float version() const { return mVersion; }
        inline QString path() const { return mPath; }
        inline QStringList masters() const { return mMasters; }
        inline QString description() const { return mDescription; }

        //inline ModelItem *parent() const { return ModelItem::parent(); this->}

    private:
        QString mFileName;
        QString mAuthor;
        int mSize;
        QDateTime mModified;
        QDateTime mAccessed;
        float mVersion;
        QString mPath;
        QStringList mMasters;
        QString mDescription;

    };
}

Q_DECLARE_METATYPE (EsxModel::EsmFile *)

#endif
