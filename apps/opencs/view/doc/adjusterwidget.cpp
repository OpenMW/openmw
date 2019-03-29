#include "adjusterwidget.hpp"

#include <components/misc/stringops.hpp>

#include <boost/filesystem.hpp>

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

CSVDoc::AdjusterWidget::AdjusterWidget (QWidget *parent)
    : QWidget (parent), mValid (false), mAction (ContentAction_Undefined)
{
    QHBoxLayout *layout = new QHBoxLayout (this);

    mIcon = new QLabel (this);

    layout->addWidget (mIcon, 0);

    mMessage = new QLabel (this);
    mMessage->setWordWrap (true);
    mMessage->setSizePolicy (QSizePolicy (QSizePolicy::Minimum, QSizePolicy::Minimum));

    layout->addWidget (mMessage, 1);

    setName ("", false);

    setLayout (layout);
}

void CSVDoc::AdjusterWidget::setAction (ContentAction action)
{
    mAction = action;
}

void CSVDoc::AdjusterWidget::setLocalData (const boost::filesystem::path& localData)
{
    mLocalData = localData;
}

boost::filesystem::path CSVDoc::AdjusterWidget::getPath() const
{
    if (!mValid)
        throw std::logic_error ("invalid content file path");

    return mResultPath;
}

bool CSVDoc::AdjusterWidget::isValid() const
{
    return mValid;
}

void CSVDoc::AdjusterWidget::setFilenameCheck (bool doCheck)
{
    mDoFilenameCheck = doCheck;
}

void CSVDoc::AdjusterWidget::setName (const QString& name, bool addon)
{
    QString message;

    mValid = (!name.isEmpty());
    bool warning = false;

    if (!mValid)
    {
        message = "No name.";
    }
    else
    {
        boost::filesystem::path path (name.toUtf8().data());

        std::string extension = Misc::StringUtils::lowerCase(path.extension().string());

        bool isLegacyPath = (extension == ".esm" ||
                             extension == ".esp");

        bool isFilePathChanged = (path.parent_path().string() != mLocalData.string());

        if (isLegacyPath)
            path.replace_extension (addon ? ".omwaddon" : ".omwgame");

        //if the file came from data-local and is not a legacy file to be converted,
        //don't worry about doing a file check.
        if (!isFilePathChanged && !isLegacyPath)
        {
            // path already points to the local data directory
            message = QString::fromUtf8 (("Will be saved as: " + path.string()).c_str());
            mResultPath = path;
        }
        //in all other cases, ensure the path points to data-local and do an existing file check
        else
        {
            // path points somewhere else or is a leaf name.
            if (isFilePathChanged)
                path = mLocalData / path.filename();

            message = QString::fromUtf8 (("Will be saved as: " + path.string()).c_str());
            mResultPath = path;

            if (boost::filesystem::exists (path))
            {
                /// \todo add an user setting to make this an error.
                message += "<p>A file with the same name already exists. If you continue, it will be overwritten.";
                warning = true;
            }
        }
    }

    mMessage->setText (message);
    mIcon->setPixmap (style()->standardIcon (
        mValid ? (warning ? QStyle::SP_MessageBoxWarning : QStyle::SP_MessageBoxInformation) : QStyle::SP_MessageBoxCritical).
        pixmap (QSize (16, 16)));

    emit stateChanged (mValid);
}
