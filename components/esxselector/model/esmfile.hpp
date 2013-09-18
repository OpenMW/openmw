#ifndef ESMFILE_HPP
#define ESMFILE_HPP

#include <QDateTime>
#include <QStringList>

#include "modelitem.hpp"

class QMimeData;

namespace EsxModel
{
    enum EsmFileProperty
    {
        Property_FileName       = 0,
        Property_Author         = 1,
        Property_Version        = 2,
        Property_DateModified   = 3,
        Property_Path           = 4,
        Property_Description    = 5,
        Property_Master         = 6
    };

    class EsmFile : public ModelItem
    {
        Q_OBJECT
        Q_PROPERTY(QString filename READ fileName)

    public:

        EsmFile(QString fileName = QString(), ModelItem *parent = 0);
     //   EsmFile(const EsmFile &);

        ~EsmFile()
        {}

        void setProperty (const EsmFileProperty prop, const QString &value);

        void setFileName(const QString &fileName);
        void setAuthor(const QString &author);
        void setSize(const int size);
        void setDate(const QDateTime &modified);
        void setVersion(const float version);
        void setPath(const QString &path);
        void setMasters(const QStringList &masters);
        void setDescription(const QString &description);

        inline QString fileName() const { return mFileName; }
        inline QString author() const { return mAuthor; }
        inline QDateTime modified() const { return mModified; }
        inline float version() const { return mVersion; }
        inline QString path() const { return mPath; }
        inline QStringList masters() const { return mMasters; }
        inline QString description() const { return mDescription; }

        inline bool isMaster() const { return (mMasters.size() == 0); }
        QByteArray encodedData() const;

    public:
        static int sPropertyCount;

    private:

        QString mFileName;
        QString mAuthor;
        QDateTime mModified;
        float mVersion;
        QString mPath;
        QStringList mMasters;
        QString mDescription;

    };
}

Q_DECLARE_METATYPE (EsxModel::EsmFile *)

#endif
