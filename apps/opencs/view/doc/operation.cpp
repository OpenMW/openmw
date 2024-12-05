#include "operation.hpp"

#include <sstream>
#include <string>

#include <QHBoxLayout>
#include <QProgressBar>
#include <QPushButton>

#include "../../model/doc/state.hpp"

void CSVDoc::Operation::updateLabel(int threads)
{
    if (threads == -1 || ((threads == 0) != mStalling))
    {
        std::string name("Unknown operation");

        switch (mType)
        {
            case CSMDoc::State_Saving:
                name = "Saving";
                break;
            case CSMDoc::State_Verifying:
                name = "Verifying";
                break;
            case CSMDoc::State_Searching:
                name = "Searching";
                break;
            case CSMDoc::State_Merging:
                name = "Merging";
                break;
        }

        std::ostringstream stream;

        if ((mStalling = (threads <= 0)))
        {
            stream << name << " (waiting for a free worker thread)";
        }
        else
        {
            stream << name << " (%p%)";
        }

        mProgressBar->setFormat(stream.str().c_str());
    }
}

CSVDoc::Operation::Operation(int type, QWidget* parent)
    : mType(type)
    , mStalling(false)
{
    /// \todo Add a cancel button or a pop up menu with a cancel item
    initWidgets();
    setBarColor(type);
    updateLabel();

    /// \todo assign different progress bar colours to allow the user to distinguish easily between operation types
}

CSVDoc::Operation::~Operation()
{
    delete mLayout;
    delete mProgressBar;
    delete mAbortButton;
}

void CSVDoc::Operation::initWidgets()
{
    mProgressBar = new QProgressBar();
    mAbortButton = new QPushButton("Abort");
    mLayout = new QHBoxLayout();
    mLayout->setContentsMargins(8, 4, 8, 4);

    mLayout->addWidget(mProgressBar);
    mLayout->addWidget(mAbortButton);

    connect(mAbortButton, &QPushButton::clicked, this, qOverload<>(&Operation::abortOperation));
}

void CSVDoc::Operation::setProgress(int current, int max, int threads)
{
    updateLabel(threads);
    mProgressBar->setRange(0, max);
    mProgressBar->setValue(current);
}

int CSVDoc::Operation::getType() const
{
    return mType;
}

void CSVDoc::Operation::setBarColor(int type)
{
    QString style
        = "QProgressBar {"
          "text-align: center;"
          "border: 1px solid #4e4e4e;"
          "}"
          "QProgressBar::chunk {"
          "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:.50 %2 stop: .51 %3 stop:1 %4);"
          "text-align: center;"
          "margin: 2px;"
          "}";

    QString topColor = "#9e9e9e";
    QString bottomColor = "#919191";
    QString midTopColor = "#848484";
    QString midBottomColor = "#717171"; // default gray

    // colors inspired by samples from:
    // http://www.colorzilla.com/gradient-editor/

    switch (type)
    {
        case CSMDoc::State_Saving:

            topColor = "#f27d6e";
            midTopColor = "#ee6954";
            midBottomColor = "#f05536";
            bottomColor = "#de511e"; // red
            break;

        case CSMDoc::State_Searching:

            topColor = "#6db3f2";
            midTopColor = "#54a3ee";
            midBottomColor = "#3690f0";
            bottomColor = "#1e69de"; // blue
            break;

        case CSMDoc::State_Verifying:

            topColor = "#bfd255";
            midTopColor = "#8eb92a";
            midBottomColor = "#72aa00";
            bottomColor = "#9ecb2d"; // green
            break;

        case CSMDoc::State_Merging:

            topColor = "#d89188";
            midTopColor = "#d07f72";
            midBottomColor = "#cc6d5a";
            bottomColor = "#b86344"; // brown
            break;
    }

    mProgressBar->setStyleSheet(style.arg(topColor).arg(midTopColor).arg(midBottomColor).arg(bottomColor));
}

QHBoxLayout* CSVDoc::Operation::getLayout() const
{
    return mLayout;
}

void CSVDoc::Operation::abortOperation()
{
    emit abortOperation(mType);
}
