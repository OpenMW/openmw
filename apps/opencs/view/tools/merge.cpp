
#include "merge.hpp"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QSplitter>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QKeyEvent>

#include "../../model/doc/document.hpp"
#include "../../model/doc/documentmanager.hpp"

#include "../doc/filewidget.hpp"
#include "../doc/adjusterwidget.hpp"

void CSVTools::Merge::keyPressEvent (QKeyEvent *event)
{
    if (event->key()==Qt::Key_Escape)
    {
        event->accept();
        cancel();
    }
    else
        QWidget::keyPressEvent (event);
}

CSVTools::Merge::Merge (CSMDoc::DocumentManager& documentManager, QWidget *parent)
: QWidget (parent), mDocument (nullptr), mDocumentManager (documentManager)
{
    setWindowTitle ("Merge Content Files into a new Game File");

    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout (mainLayout);

    QSplitter *splitter = new QSplitter (Qt::Horizontal, this);

    mainLayout->addWidget (splitter, 1);

    // left panel (files to be merged)
    QWidget *left = new QWidget (this);
    left->setContentsMargins (0, 0, 0, 0);
    splitter->addWidget (left);

    QVBoxLayout *leftLayout = new QVBoxLayout;
    left->setLayout (leftLayout);

    leftLayout->addWidget (new QLabel ("Files to be merged", this));

    mFiles = new QListWidget (this);
    leftLayout->addWidget (mFiles, 1);

    // right panel (new game file)
    QWidget *right = new QWidget (this);
    right->setContentsMargins (0, 0, 0, 0);
    splitter->addWidget (right);

    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->setAlignment (Qt::AlignTop);
    right->setLayout (rightLayout);

    rightLayout->addWidget (new QLabel ("New game file", this));

    mNewFile = new CSVDoc::FileWidget (this);
    mNewFile->setType (false);
    mNewFile->extensionLabelIsVisible (true);
    rightLayout->addWidget (mNewFile);

    mAdjuster = new CSVDoc::AdjusterWidget (this);

    rightLayout->addWidget (mAdjuster);

    connect (mNewFile, SIGNAL (nameChanged (const QString&, bool)),
        mAdjuster, SLOT (setName (const QString&, bool)));
    connect (mAdjuster, SIGNAL (stateChanged (bool)), this, SLOT (stateChanged (bool)));

    // buttons
    QDialogButtonBox *buttons = new QDialogButtonBox (QDialogButtonBox::Cancel, Qt::Horizontal, this);

    connect (buttons->button (QDialogButtonBox::Cancel), SIGNAL (clicked()), this, SLOT (cancel()));

    mOkay = new QPushButton ("Merge", this);
    connect (mOkay, SIGNAL (clicked()), this, SLOT (accept()));
    mOkay->setDefault (true);
    buttons->addButton (mOkay, QDialogButtonBox::AcceptRole);

    mainLayout->addWidget (buttons);
}

void CSVTools::Merge::configure (CSMDoc::Document *document)
{
    mDocument = document;

    mNewFile->setName ("");

    // content files
    while (mFiles->count())
        delete mFiles->takeItem (0);

    std::vector<boost::filesystem::path> files = document->getContentFiles();

    for (std::vector<boost::filesystem::path>::const_iterator iter (files.begin());
        iter!=files.end(); ++iter)
        mFiles->addItem (QString::fromUtf8 (iter->filename().string().c_str()));
}

void CSVTools::Merge::setLocalData (const boost::filesystem::path& localData)
{
    mAdjuster->setLocalData (localData);
}

CSMDoc::Document *CSVTools::Merge::getDocument() const
{
    return mDocument;
}

void CSVTools::Merge::cancel()
{
    mDocument = nullptr;
    hide();
}

void CSVTools::Merge::accept()
{
    if ((mDocument->getState() & CSMDoc::State_Merging)==0)
    {
        std::vector< boost::filesystem::path > files (1, mAdjuster->getPath());

        std::unique_ptr<CSMDoc::Document> target (
            mDocumentManager.makeDocument (files, files[0], true));

        mDocument->runMerge (std::move(target));

        hide();
    }
}

void CSVTools::Merge::stateChanged (bool valid)
{
    mOkay->setEnabled (valid);
}
