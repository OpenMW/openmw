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

#include <QDebug>

CSVDoc::FileDialog::FileDialog(QWidget *parent) :
    QDialog(parent), mSelector (0), mFileWidget (0), mAdjusterWidget (0)
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

QStringList CSVDoc::FileDialog::selectedFilePaths()
{
    QStringList filePaths;

    foreach (ContentSelectorModel::EsmFile *file, mSelector->selectedFiles() )
        filePaths.append(file->path());

    return filePaths;
}

void CSVDoc::FileDialog::setLocalData (const boost::filesystem::path& localData)
{
    if (mDialogType != DialogType_New)
        return;

    mAdjusterWidget->setLocalData (localData);
}

void CSVDoc::FileDialog::showDialog(DialogType dialogType)
{
    mDialogType = dialogType;

    switch (mDialogType)
    {
    case DialogType_New:
        buildNewFileView();
        break;

    case DialogType_Open:
        buildOpenFileView();
        break;
    default:
        break;
    }

    //connections common to both dialog view flavors
    connect (mSelector, SIGNAL (signalCurrentGamefileIndexChanged (int)),
             this, SLOT (slotUpdateAcceptButton (int)));

    connect (ui.projectButtonBox, SIGNAL (rejected()), this, SLOT (slotRejected()));

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

    mFileWidget = new FileWidget (this);

    mFileWidget->setType (true);
    mFileWidget->extensionLabelIsVisible(true);

    ui.projectGroupBoxLayout->insertWidget (0, mFileWidget);
    ui.projectGroupBoxLayout->insertWidget (1, mAdjusterWidget);

    connect (mFileWidget, SIGNAL (nameChanged (const QString&, bool)),
        mAdjusterWidget, SLOT (setName (const QString&, bool)));

    connect (mFileWidget, SIGNAL (nameChanged(const QString &, bool)),
            this, SLOT (slotUpdateAcceptButton(const QString &, bool)));

    connect (ui.projectButtonBox, SIGNAL (accepted()), this, SLOT (slotNewFile()));
}

void CSVDoc::FileDialog::buildOpenFileView()
{
    setWindowTitle(tr("Open"));
    ui.projectGroupBox->setTitle (QString(""));
    ui.projectGroupBox->layout()->addWidget (mAdjusterWidget);

    mAdjusterWidget->setVisible (false);

    ui.projectButtonBox->button(QDialogButtonBox::Ok)->setEnabled (false);

    connect (ui.projectButtonBox, SIGNAL (accepted()), this, SLOT (slotOpenFile()));
}

void CSVDoc::FileDialog::slotUpdateAcceptButton (int)
{
    QString name = "";

    if (mDialogType == DialogType_New)
        name = mFileWidget->getName();

    slotUpdateAcceptButton (name, true);
}

void CSVDoc::FileDialog::slotUpdateAcceptButton(const QString &name, bool)
{
    bool success = (mSelector->selectedFiles().size() > 0);

    if (mDialogType == DialogType_New)
        success = success && !(name.isEmpty());

    ui.projectButtonBox->button (QDialogButtonBox::Ok)->setEnabled (success);
}

QString CSVDoc::FileDialog::filename() const
{
    if (mDialogType == DialogType_New)
        return "";

    return mSelector->currentFile();
}

void CSVDoc::FileDialog::slotRejected()
{
    emit rejected();
    close();
}

void CSVDoc::FileDialog::slotNewFile()
{
    emit signalCreateNewFile (mAdjusterWidget->getPath());
}

void CSVDoc::FileDialog::slotOpenFile()
{
    ContentSelectorModel::EsmFile *file = mSelector->selectedFiles().back();

    mAdjusterWidget->setName (file->fileName(), !file->isGameFile());

    emit signalOpenFiles (mAdjusterWidget->getPath());
}
