#ifndef ESMFILE_HPP
#define ESMFILE_HPP

#include <QDateTime>
#include <QStringList>

#include "modelitem.hpp"

class QMimeData;

namespace ContentSelectorModel
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
            FileProperty_Format         = 2,
            FileProperty_DateModified   = 3,
            FileProperty_Path           = 4,
            FileProperty_Description    = 5,
            FileProperty_GameFile       = 6
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
        void setFormat(const int format);
        void setPath(const QString &path);
        void setGameFiles(const QStringList &gameFiles);
        void setDescription(const QString &description);

        inline void addGameFile (const QString &name) {mGameFiles.append(name); }
        QVariant fileProperty (const FileProperty prop) const;

        inline QString fileName() const             { return mFileName; }
        inline QString author() const               { return mAuthor; }
        inline QDateTime modified() const           { return mModified; }
        inline float format() const                 { return mFormat; }
        inline QString path() const                 { return mPath; }
        inline const QStringList &gameFiles() const { return mGameFiles; }
        inline QString description() const          { return mDescription; }
        inline QString toolTip() const              { return sToolTip.arg(mAuthor)
                                                             .arg(mFormat)
                                                             .arg(mDescription)
                                                             .arg(mGameFiles.join(", "));
                                                    }

        inline bool isGameFile() const              { return (mGameFiles.size() == 0); }
        QByteArray encodedData() const;

    public:
        static int sPropertyCount;
        static QString sToolTip;

    private:

        QString mFileName;
        QString mAuthor;
        QDateTime mModified;
        int mFormat;
        QString mPath;
        QStringList mGameFiles;
        QString mDescription;
        QString mToolTip;

    };
}

#endif
