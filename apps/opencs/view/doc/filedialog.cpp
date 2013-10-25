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
    QDialog(parent), mSelector (0)
{
    ui.setupUi (this);
    resize(400, 400);

    setObjectName ("FileDialog");
    mSelector = new ContentSelectorView::ContentSelector (ui.contentSelectorWidget);
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

    connect (mFileWidget, SIGNAL (nameChanged(const QString &, bool)),
            this, SLOT (slotUpdateCreateButton(const QString &, bool)));

    connect (mSelector, SIGNAL (signalCurrentGamefileIndexChanged (int)),
             this, SLOT (slotUpdateCreateButton (int)));

    connect (ui.projectButtonBox, SIGNAL (accepted()), this, SIGNAL (createNewFile()));
    connect (ui.projectButtonBox, SIGNAL (rejected()), this, SLOT (slotRejected()));
}

void CSVDoc::FileDialog::buildOpenFileView()
{
    setWindowTitle(tr("Open"));
    ui.projectGroupBox->setTitle (QString(""));

    connect (ui.projectButtonBox, SIGNAL (accepted()), this, SIGNAL (openFiles()));
    connect (ui.projectButtonBox, SIGNAL (rejected()), this, SLOT (slotRejected()));
}

void CSVDoc::FileDialog::slotUpdateCreateButton (int)
{
    slotUpdateCreateButton (mFileWidget->getName(), true);
}

void CSVDoc::FileDialog::slotUpdateCreateButton(const QString &name, bool)
{
    if (!(mDialogType == DialogType_New))
        return;

    bool success = (!name.isEmpty() && mSelector->selectedFiles().size() > 0);

    ui.projectButtonBox->button (QDialogButtonBox::Ok)->setEnabled (success);
}

QString CSVDoc::FileDialog::filename() const
{
    if (mDialogType == DialogType_New)
        return mFileWidget->getName();

    return mSelector->currentFile();
}

void CSVDoc::FileDialog::slotRejected()
{
    emit rejected();
    close();
}
