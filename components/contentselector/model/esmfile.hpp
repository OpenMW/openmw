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
            FileProperty_BuiltIn = 6,
            FileProperty_FromAnotherConfigFile = 7,
            FileProperty_GameFile = 8,
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
        void setBuiltIn(bool builtIn);
        void setFromAnotherConfigFile(bool fromAnotherConfigFile);

        void addGameFile(const QString& name) { mGameFiles.append(name); }
        QVariant fileProperty(const FileProperty prop) const;

        const QString& fileName() const { return mFileName; }
        const QString& author() const { return mAuthor; }
        QDateTime modified() const { return mModified; }
        const QString& formatVersion() const { return mVersion; }
        const QString& filePath() const { return mPath; }
        bool builtIn() const { return mBuiltIn; }
        bool fromAnotherConfigFile() const { return mFromAnotherConfigFile; }
        bool isMissing() const { return mPath.isEmpty(); }

        /// @note Contains file names, not paths.
        const QStringList& gameFiles() const { return mGameFiles; }
        const QString& description() const { return mDescription; }
        QString toolTip() const
        {
            if (isMissing())
                return tr("<b>This file is specified in a non-user config file, but does not exist in the VFS.</b>");
            QString tooltip = mTooltipTemlate.arg(mAuthor)
                                  .arg(mVersion)
                                  .arg(mModified.toString(Qt::ISODate))
                                  .arg(mPath)
                                  .arg(mDescription)
                                  .arg(mGameFiles.join(", "));

            if (mBuiltIn)
                tooltip += tr("<br/><b>This content file cannot be disabled because it is part of OpenMW.</b><br/>");
            else if (mFromAnotherConfigFile)
                tooltip += tr(
                    "<br/><b>This content file cannot be disabled because it is enabled in a config file other than "
                    "the user one.</b><br/>");

            return tooltip;
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
        bool mBuiltIn = false;
        bool mFromAnotherConfigFile = false;
        bool mHasGameExtension = false;
    };
}

#endif
