#ifndef ESMFILE_HPP
#define ESMFILE_HPP

#include <QDateTime>
#include <QStringList>

#include "modelitem.hpp"

class QMimeData;

namespace EsxModel
{
    class EsmFile : public ModelItem
    {
        Q_OBJECT
        Q_PROPERTY(QString filename READ fileName)

    public:

        enum FileProperty
        {
            FileProperty_FileName       = 0,
            FileProperty_Author         = 1,
            FileProperty_Version        = 2,
            FileProperty_DateModified   = 3,
            FileProperty_Path           = 4,
            FileProperty_Description    = 5,
            FileProperty_Master         = 6
        };

        EsmFile(QString fileName = QString(), ModelItem *parent = 0);
     //   EsmFile(const EsmFile &);

        ~EsmFile()
        {}

        void setFileProperty (const FileProperty prop, const QString &value);

        void setFileName(const QString &fileName);
        void setAuthor(const QString &author);
        void setSize(const int size);
        void setDate(const QDateTime &modified);
        void setVersion(const float version);
        void setPath(const QString &path);
        void setMasters(const QStringList &masters);
        void setDescription(const QString &description);

        inline void addMaster (const QString &name) {mMasters.append(name); }
        QVariant fileProperty (const FileProperty prop) const;

        inline QString fileName() const     { return mFileName; }
        inline QString author() const       { return mAuthor; }
        inline QDateTime modified() const   { return mModified; }
        inline float version() const        { return mVersion; }
        inline QString path() const         { return mPath; }
        inline const QStringList &masters() const { return mMasters; }
        inline QString description() const  { return mDescription; }
        inline QString toolTip() const      { return sToolTip.arg(mAuthor)
                                                             .arg(mVersion)
                                                             .arg(mDescription)
                                                             .arg(mMasters.join(", "));
                                            }

        inline bool isMaster() const        { return (mMasters.size() == 0); }
        QByteArray encodedData() const;

    public:
        static int sPropertyCount;
        static QString sToolTip;

    private:

        QString mFileName;
        QString mAuthor;
        QDateTime mModified;
        float mVersion;
        QString mPath;
        QStringList mMasters;
        QString mDescription;
        QString mToolTip;

    };
}

Q_DECLARE_METATYPE (EsxModel::EsmFile *)

#endif
