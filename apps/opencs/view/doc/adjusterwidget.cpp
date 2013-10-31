
#include "adjusterwidget.hpp"

#include <stdexcept>
#include <string>

#include <boost/filesystem.hpp>

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

CSVDoc::AdjusterWidget::AdjusterWidget (QWidget *parent)
: QWidget (parent), mValid (false)
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

void CSVDoc::AdjusterWidget::setName (const QString& name, bool addon)
{
    QString message;

    if (name.isEmpty())
    {
        mValid = false;
        message = "No name.";
    }
    else
    {
        boost::filesystem::path path (name.toUtf8().data());

        path.replace_extension (addon ? ".omwaddon" : ".omwgame");

        if (path.parent_path().string()==mLocalData.string())
        {
            // path already points to the local data directory
            message = QString::fromUtf8 (("Will be saved as: " + path.string()).c_str());
            mResultPath = path;
            mValid = true;
        }
        else
        {
            // path points somewhere else or is a leaf name.
            path = mLocalData / path.filename();

            message = QString::fromUtf8 (("Will be saved as: " + path.string()).c_str());
            mResultPath = path;
            mValid = true;

            if (boost::filesystem::exists (path))
            {
                /// \todo add an user setting to make this an error.
                message += "<p>But a file with the same name already exists. If you continue, it will be overwritten.";
            }
        }
    }

    mMessage->setText (message);
    mIcon->setPixmap (style()->standardIcon (
        mValid ? QStyle::SP_MessageBoxInformation : QStyle::SP_MessageBoxWarning).
        pixmap (QSize (16, 16)));

    emit stateChanged (mValid);
}