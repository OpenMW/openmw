#include "filedialog.hpp"

#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSortFilterProxyModel>
#include <QRegExpValidator>
#include <QRegExp>
#include <QSpacerItem>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>

#include "components/contentselector/model/esmfile.hpp"
#include "components/contentselector/view/contentselector.hpp"

#include "filewidget.hpp"
#include "adjusterwidget.hpp"

CSVDoc::FileDialog::FileDialog(QWidget *parent) :
    QDialog(parent), mSelector (0), mAction(ContentAction_Undefined), mFileWidget (0), mAdjusterWidget (0), mDialogBuilt(false)
{
    ui.setupUi (this);
    resize(400, 400);

    setObjectName ("FileDialog");
    mSelector = new ContentSelectorView::ContentSelector (ui.contentSelectorWidget);
    mAdjusterWidget = new AdjusterWidget (this);
}

void CSVDoc::FileDialog::addFiles(const QString &path)
{
    mSelector->addFiles(path);
}

void CSVDoc::FileDialog::setEncoding(const QString &encoding)
{
    mSelector->setEncoding(encoding);
}

void CSVDoc::FileDialog::clearFiles()
{
    mSelector->clearFiles();
}

QStringList CSVDoc::FileDialog::selectedFilePaths()
{
    QStringList filePaths;

    for (ContentSelectorModel::EsmFile *file : mSelector->selectedFiles() )
        filePaths.append(file->filePath());

    return filePaths;
}

void CSVDoc::FileDialog::setLocalData (const boost::filesystem::path& localData)
{
    mAdjusterWidget->setLocalData (localData);
}

void CSVDoc::FileDialog::showDialog (ContentAction action)
{
    mAction = action;

    ui.projectGroupBoxLayout->insertWidget (0, mAdjusterWidget);

    switch (mAction)
    {
    case ContentAction_New:
        buildNewFileView();
        break;

    case ContentAction_Edit:
        buildOpenFileView();
        break;

    default:
        break;
    }

    mAdjusterWidget->setFilenameCheck (mAction == ContentAction_New);

    if(!mDialogBuilt)
    {
        //connections common to both dialog view flavors
        connect (mSelector, SIGNAL (signalCurrentGamefileIndexChanged (int)),
                 this, SLOT (slotUpdateAcceptButton (int)));

        connect (ui.projectButtonBox, SIGNAL (rejected()), this, SLOT (slotRejected()));
        mDialogBuilt = true;
    }

    show();
    raise();
    activateWindow();
}

void CSVDoc::FileDialog::buildNewFileView()
{
    setWindowTitle(tr("Create a new addon"));

    QPushButton* createButton = ui.projectButtonBox->button (QDialogButtonBox::Ok);
    createButton->setText ("Create");
    createButton->setEnabled (false);

    if(!mFileWidget)
    {
        mFileWidget = new FileWidget (this);

        mFileWidget->setType (true);
        mFileWidget->extensionLabelIsVisible(true);

        connect (mFileWidget, SIGNAL (nameChanged (const QString&, bool)),
            mAdjusterWidget, SLOT (setName (const QString&, bool)));

        connect (mFileWidget, SIGNAL (nameChanged(const QString &, bool)),
                this, SLOT (slotUpdateAcceptButton(const QString &, bool)));
    }

    ui.projectGroupBoxLayout->insertWidget (0, mFileWidget);

    connect (ui.projectButtonBox, SIGNAL (accepted()), this, SLOT (slotNewFile()));
}

void CSVDoc::FileDialog::buildOpenFileView()
{
    setWindowTitle(tr("Open"));
    ui.projectGroupBox->setTitle (QString(""));
    ui.projectButtonBox->button(QDialogButtonBox::Ok)->setText ("Open");
    if(mSelector->isGamefileSelected())
        ui.projectButtonBox->button(QDialogButtonBox::Ok)->setEnabled (true);
    else
        ui.projectButtonBox->button(QDialogButtonBox::Ok)->setEnabled (false);

    if(!mDialogBuilt)
    {
        connect (mSelector, SIGNAL (signalAddonDataChanged (const QModelIndex&, const QModelIndex&)), this, SLOT (slotAddonDataChanged(const QModelIndex&, const QModelIndex&)));
    }
        connect (ui.projectButtonBox, SIGNAL (accepted()), this, SLOT (slotOpenFile()));
}

void CSVDoc::FileDialog::slotAddonDataChanged(const QModelIndex &topleft, const QModelIndex &bottomright)
{
    slotUpdateAcceptButton(0);
}

void CSVDoc::FileDialog::slotUpdateAcceptButton(int)
{
    QString name = "";

    if (mFileWidget && mAction == ContentAction_New)
        name = mFileWidget->getName();

    slotUpdateAcceptButton (name, true);
}

void CSVDoc::FileDialog::slotUpdateAcceptButton(const QString &name, bool)
{
    bool success = !mSelector->selectedFiles().empty();

    bool isNew = (mAction == ContentAction_New);

    if (isNew)
        success = success && !(name.isEmpty());
    else if (success)
    {
        ContentSelectorModel::EsmFile *file = mSelector->selectedFiles().back();
        mAdjusterWidget->setName (file->filePath(), !file->isGameFile());
    }
    else
        mAdjusterWidget->setName ("", true);

    ui.projectButtonBox->button (QDialogButtonBox::Ok)->setEnabled (success);
}

QString CSVDoc::FileDialog::filename() const
{
    if (mAction == ContentAction_New)
        return "";

    return mSelector->currentFile();
}

void CSVDoc::FileDialog::slotRejected()
{
    emit rejected();
    disconnect (ui.projectButtonBox, SIGNAL (accepted()), this, SLOT (slotNewFile()));
    disconnect (ui.projectButtonBox, SIGNAL (accepted()), this, SLOT (slotOpenFile()));
    if(mFileWidget)
    {
        delete mFileWidget;
        mFileWidget = nullptr;
    }
    close();
}

void CSVDoc::FileDialog::slotNewFile()
{
    emit signalCreateNewFile (mAdjusterWidget->getPath());
    if(mFileWidget)
    {
        delete mFileWidget;
        mFileWidget = nullptr;
    }
    disconnect (ui.projectButtonBox, SIGNAL (accepted()), this, SLOT (slotNewFile()));
    close();
}

void CSVDoc::FileDialog::slotOpenFile()
{
    ContentSelectorModel::EsmFile *file = mSelector->selectedFiles().back();

    mAdjusterWidget->setName (file->filePath(), !file->isGameFile());

    emit signalOpenFiles (mAdjusterWidget->getPath());
    disconnect (ui.projectButtonBox, SIGNAL (accepted()), this, SLOT (slotOpenFile()));
    close();
}
