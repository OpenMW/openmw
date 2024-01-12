#ifndef ESMFILE_HPP
#define ESMFILE_HPP

#include <QDateTime>
#include <QStringList>

#include <components/esm3/formatversion.hpp>

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
            FileProperty_FileName = 0,
            FileProperty_Author = 1,
            FileProperty_Format = 2,
            FileProperty_DateModified = 3,
            FileProperty_FilePath = 4,
            FileProperty_Description = 5,
            FileProperty_GameFile = 6
        };

        EsmFile(const QString& fileName = QString(), ModelItem* parent = nullptr);

        void setFileProperty(const FileProperty prop, const QString& value);

        void setFileName(const QString& fileName);
        void setAuthor(const QString& author);
        void setDate(const QDateTime& modified);
        void setFormat(const QString& format);
        void setFilePath(const QString& path);
        void setGameFiles(const QStringList& gameFiles);
        void setDescription(const QString& description);

        void addGameFile(const QString& name) { mGameFiles.append(name); }
        QVariant fileProperty(const FileProperty prop) const;

        QString fileName() const { return mFileName; }
        QString author() const { return mAuthor; }
        QDateTime modified() const { return mModified; }
        QString formatVersion() const { return mVersion; }
        QString filePath() const { return mPath; }

        /// @note Contains file names, not paths.
        const QStringList& gameFiles() const { return mGameFiles; }
        QString description() const { return mDescription; }
        QString toolTip() const
        {
            return mTooltipTemlate.arg(mAuthor)
                .arg(mVersion)
                .arg(mModified.toString(Qt::ISODate))
                .arg(mPath)
                .arg(mDescription)
                .arg(mGameFiles.join(", "));
        }

        bool isGameFile() const;

    private:
        QString mTooltipTemlate = tr(
            "<b>Author:</b> %1<br/>"
            "<b>Format version:</b> %2<br/>"
            "<b>Modified:</b> %3<br/>"
            "<b>Path:</b><br/>%4<br/>"
            "<br/><b>Description:</b><br/>%5<br/>"
            "<br/><b>Dependencies: </b>%6<br/>");

        QString mFileName;
        QString mAuthor;
        QDateTime mModified;
        QString mVersion = QString::number(ESM::DefaultFormatVersion);
        QString mPath;
        QStringList mGameFiles;
        QString mDescription;
        QString mToolTip;
    };
}

#endif
