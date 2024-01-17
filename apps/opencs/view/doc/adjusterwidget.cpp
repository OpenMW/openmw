#include "adjusterwidget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

#include <filesystem>
#include <stdexcept>
#include <string>

#include <components/files/qtconversion.hpp>
#include <components/misc/strings/lower.hpp>

CSVDoc::AdjusterWidget::AdjusterWidget(QWidget* parent)
    : QWidget(parent)
    , mValid(false)
    , mAction(ContentAction_Undefined)
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    mIcon = new QLabel(this);

    layout->addWidget(mIcon, 0);

    mMessage = new QLabel(this);
    mMessage->setWordWrap(true);
    mMessage->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));

    layout->addWidget(mMessage, 1);

    setName("", false);

    setLayout(layout);
}

void CSVDoc::AdjusterWidget::setAction(ContentAction action)
{
    mAction = action;
}

void CSVDoc::AdjusterWidget::setLocalData(const std::filesystem::path& localData)
{
    mLocalData = localData;
}

std::filesystem::path CSVDoc::AdjusterWidget::getPath() const
{
    if (!mValid)
        throw std::logic_error("invalid content file path");

    return mResultPath;
}

bool CSVDoc::AdjusterWidget::isValid() const
{
    return mValid;
}

void CSVDoc::AdjusterWidget::setFilenameCheck(bool doCheck)
{
    mDoFilenameCheck = doCheck;
}

void CSVDoc::AdjusterWidget::setName(const QString& name, bool addon)
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
        auto path = Files::pathFromQString(name);

        const auto extension = Misc::StringUtils::lowerCase(path.extension().u8string());

        bool isLegacyPath = (extension == u8".esm" || extension == u8".esp");

        bool isFilePathChanged = (path.parent_path() != mLocalData);

        if (isLegacyPath)
            path.replace_extension(addon ? ".omwaddon" : ".omwgame");

        // if the file came from data-local and is not a legacy file to be converted,
        // don't worry about doing a file check.
        if (!isFilePathChanged && !isLegacyPath)
        {
            // path already points to the local data directory
            message = "Will be saved as: " + Files::pathToQString(path);
            mResultPath = std::move(path);
        }
        // in all other cases, ensure the path points to data-local and do an existing file check
        else
        {
            // path points somewhere else or is a leaf name.
            if (isFilePathChanged)
                path = mLocalData / path.filename();

            message = "Will be saved as: " + Files::pathToQString(path);
            mResultPath = path;

            if (std::filesystem::exists(path))
            {
                /// \todo add an user setting to make this an error.
                message += "<p>A file with the same name already exists. If you continue, it will be overwritten.";
                warning = true;
            }
        }
    }

    mMessage->setText(message);
    mIcon->setPixmap(
        style()
            ->standardIcon(mValid ? (warning ? QStyle::SP_MessageBoxWarning : QStyle::SP_MessageBoxInformation)
                                  : QStyle::SP_MessageBoxCritical)
            .pixmap(QSize(16, 16)));

    emit stateChanged(mValid);
}
