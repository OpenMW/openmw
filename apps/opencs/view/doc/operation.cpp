#include "operation.hpp"

#include <sstream>

#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../model/doc/document.hpp"

void CSVDoc::Operation::updateLabel (int threads)
{
    if (threads==-1 || ((threads==0)!=mStalling))
    {
        std::string name ("unknown operation");

        switch (mType)
        {
            case CSMDoc::State_Saving: name = "saving"; break;
            case CSMDoc::State_Verifying: name = "verifying"; break;
            case CSMDoc::State_Searching: name = "searching"; break;
            case CSMDoc::State_Merging: name = "merging"; break;
        }

        std::ostringstream stream;

        if ((mStalling = (threads<=0)))
        {
            stream << name << " (waiting for a free worker thread)";
        }
        else
        {
            stream << name << " (%p%)";
        }

        mProgressBar->setFormat (stream.str().c_str());
    }
}

CSVDoc::Operation::Operation (int type, QWidget* parent) : mType (type), mStalling (false)
{
    /// \todo Add a cancel button or a pop up menu with a cancel item
    initWidgets();
    setBarColor( type);
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
    mProgressBar = new QProgressBar ();
    mAbortButton = new QPushButton("Abort");
    mLayout = new QHBoxLayout();

    mLayout->addWidget (mProgressBar);
    mLayout->addWidget (mAbortButton);

    connect (mAbortButton, SIGNAL (clicked()), this, SLOT (abortOperation()));
}

void CSVDoc::Operation::setProgress (int current, int max, int threads)
{
    updateLabel (threads);
    mProgressBar->setRange (0, max);
    mProgressBar->setValue (current);
}

int CSVDoc::Operation::getType() const
{
    return mType;
}

void CSVDoc::Operation::setBarColor (int type)
{
    QString style ="QProgressBar {"
            "text-align: center;"
            "}"
          "QProgressBar::chunk {"
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:.50 %2 stop: .51 %3 stop:1 %4);"
            "text-align: center;"
            "margin: 2px 1px 1p 2px;"
          "}";

    QString topColor = "#F2F6F8";
    QString bottomColor = "#E0EFF9";
    QString midTopColor = "#D8E1E7";
    QString midBottomColor = "#B5C6D0";  // default gray gloss

    // colors inspired by samples from:
    // http://www.colorzilla.com/gradient-editor/

    switch (type)
    {
    case CSMDoc::State_Saving:

        topColor = "#FECCB1";
        midTopColor = "#F17432";
        midBottomColor = "#EA5507";
        bottomColor = "#FB955E";  // red gloss #2
        break;

    case CSMDoc::State_Searching:

        topColor = "#EBF1F6";
        midTopColor = "#ABD3EE";
        midBottomColor = "#89C3EB";
        bottomColor = "#D5EBFB"; //blue gloss #4
        break;

    case CSMDoc::State_Verifying:

        topColor = "#BFD255";
        midTopColor = "#8EB92A";
        midBottomColor = "#72AA00";
        bottomColor = "#9ECB2D";  //green gloss
        break;

    case CSMDoc::State_Merging:

        topColor = "#F3E2C7";
        midTopColor = "#C19E67";
        midBottomColor = "#B68D4C";
        bottomColor = "#E9D4B3";  //l Brown 3D
        break;

    default:

        topColor = "#F2F6F8";
        bottomColor = "#E0EFF9";
        midTopColor = "#D8E1E7";
        midBottomColor = "#B5C6D0";  // gray gloss for undefined ops
    }

    mProgressBar->setStyleSheet(style.arg(topColor).arg(midTopColor).arg(midBottomColor).arg(bottomColor));
}

QHBoxLayout *CSVDoc::Operation::getLayout() const
{
    return mLayout;
}

void CSVDoc::Operation::abortOperation()
{
    emit abortOperation (mType);
}
