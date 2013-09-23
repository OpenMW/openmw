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

#include <components/contentselector/model/esmfile.hpp>
#include <components/contentselector/view/lineedit.hpp>

#include "filewidget.hpp"
#include "adjusterwidget.hpp"

#include <QDebug>

CSVDoc::FileDialog::FileDialog(QWidget *parent) :
    ContentSelector(parent),
    mFileWidget (new FileWidget (this)),
    mAdjusterWidget (new AdjusterWidget (this)),
    mEnable_1(false),
    mEnable_2(false)
{
    // Hide the profile elements
    profileGroupBox->hide();
    addonView->showColumn(2);

    resize(400, 400);

    mFileWidget->setType(true);
    mFileWidget->extensionLabelIsVisible(false);

    connect(projectCreateButton, SIGNAL(clicked()), this, SLOT(createNewFile()));

    connect(projectButtonBox, SIGNAL(accepted()), this, SIGNAL(openFiles()));
    connect(projectButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect (mFileWidget, SIGNAL (nameChanged (const QString&, bool)),
        mAdjusterWidget, SLOT (setName (const QString&, bool)));

    connect (mAdjusterWidget, SIGNAL (stateChanged (bool)), this, SLOT (slotAdjusterChanged(bool)));
    connect (this, SIGNAL (signalGameFileChanged(int)), this, SLOT (slotGameFileSelected(int)));
    connect (this, SIGNAL (signalUpdateCreateButton(bool, int)), this, SLOT (slotEnableCreateButton(bool, int)));
}

void CSVDoc::FileDialog::setLocalData (const boost::filesystem::path& localData)
{
    mAdjusterWidget->setLocalData (localData);
}

void CSVDoc::FileDialog::updateOpenButton(const QStringList &items)
{
    QPushButton *openButton = projectButtonBox->button(QDialogButtonBox::Open);

    if (!openButton)
        return;

    openButton->setEnabled(!items.isEmpty());
}

void CSVDoc::FileDialog::slotEnableCreateButton(bool enable, int widgetNumber)
{

    if (widgetNumber == 1)
        mEnable_1 = enable;

    if (widgetNumber == 2)
        mEnable_2 = enable;

    qDebug() << "update enabled" << mEnable_1 << mEnable_2 << enable;
    projectCreateButton->setEnabled(mEnable_1 && mEnable_2);
}

QString CSVDoc::FileDialog::fileName()
{
    return mFileWidget->getName();
}

void CSVDoc::FileDialog::openFile()
{
    setWindowTitle(tr("Open"));

    mFileWidget->hide();
    adjusterWidgetFrame->hide();
    projectCreateButton->hide();
    projectGroupBox->setTitle(tr(""));
    projectButtonBox->button(QDialogButtonBox::Open)->setEnabled(false);

    show();
    raise();
    activateWindow();
}

void CSVDoc::FileDialog::newFile()
{
    setWindowTitle(tr("New"));

    fileWidgetFrame->layout()->addWidget(mFileWidget);
    adjusterWidgetFrame->layout()->addWidget(mAdjusterWidget);

    projectButtonBox->setStandardButtons(QDialogButtonBox::Cancel);
    projectButtonBox->addButton(projectCreateButton, QDialogButtonBox::ActionRole);

    show();
    raise();
    activateWindow();
}

void CSVDoc::FileDialog::slotAdjusterChanged(bool value)
{
    emit signalUpdateCreateButton(mAdjusterWidget->isValid(), 2);
}

void CSVDoc::FileDialog::slotGameFileSelected(int value)
{
    emit signalUpdateCreateButton(value > -1, 1);
}

void CSVDoc::FileDialog::createNewFile()
{
    emit createNewFile (mAdjusterWidget->getPath());
}