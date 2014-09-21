#include "settingwindow.hpp"

#include <QApplication>
//#include <QDesktopWidget>
//#include <QLineEdit>
//#include <QRadioButton>
#include <QDebug>

#include "../../model/settings/setting.hpp"
#include "../../model/settings/connector.hpp"
#include "../../model/settings/usersettings.hpp"
#include "page.hpp"
#include "view.hpp"

CSVSettings::SettingWindow::SettingWindow(QTabWidget *parent)
    : QTabWidget(parent)
{}

void CSVSettings::SettingWindow::createPages()
{
    CSMSettings::SettingPageMap pageMap = mModel->settingPageMap();

    QList <CSMSettings::Setting *> connectedSettings;

    foreach (const QString &pageName, pageMap.keys())
    {
        QList <CSMSettings::Setting *> pageSettings = pageMap.value (pageName);

        mPages.append (new Page (pageName, pageSettings, this));

        for (int i = 0; i < pageSettings.size(); i++)
        {
            CSMSettings::Setting *setting = pageSettings.at(i);

            if (!setting->proxyLists().isEmpty())
                connectedSettings.append (setting);
        }
    }

    if (!connectedSettings.isEmpty())
        createConnections(connectedSettings);
}

void CSVSettings::SettingWindow::buildTabPage(Page* tab)
{
    CSMSettings::SettingPageMap pageMap = mModel->settingPageMap();
    QList <CSMSettings::Setting *> pageSettings = pageMap.value (tab->objectName());
    //tab->setupViews(pageSettings);
#if 0
    QFontMetrics fm (QApplication::font());
    //int textWidth = fm.width(pageName); // FIXME:
    QString pageName = tab->objectName();

    std::cout << "pageName: " + pageName.toStdString() << std::endl; // FIXME:
    QList<CSMSettings::Setting*> settingList = mModel->settingPageMap.value(pageName);
    //foreach (View *view, page->views())
    //{
        //std::cout << "view key: " + view->viewKey().toStdString() << std::endl;
        //if(!settingList.empty())
        //{
    QGroupBox *section = new QGroupBox(tab);
    section->setGeometry(5, 5, this->width()-15, this->height()-15); // FIXME: frame thickness calculation
    // FIXME: assume a single grid for now
    QGridLayout *gridLayout = new QGridLayout(section);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    gridLayout->setContentsMargins(5, 5, 5, 5);

    foreach(CSMSettings::Setting * setting, settingList)
    {
        // enum ViewType {ViewType_Boolean = 0,
        //                ViewType_List = 1,
        //                ViewType_Range = 2,
        //                ViewType_Text = 3,
        //                ViewType_Undefined = 4 };
        // enum SettingType {* 0 - 9 - Boolean widgets
        //                   * 10-19 - List widgets
        //                   * 21-29 - Range widgets
        //                   * 31-39 - Text widgets
        //                   *
        //                   * Each range corresponds to a View_Type enum by a factor of 10.
        //                   *
        //                   * Even-numbered values are single-value widgets
        //                   * Odd-numbered values are multi-valued widgets
        //
        //                 Type_CheckBox = 0,
        //                 Type_RadioButton = 1,
        //                 Type_ListView = 10,
        //                 Type_ComboBox = 11,
        //                 Type_SpinBox = 21,
        //                 Type_DoubleSpinBox = 23,
        //                 Type_Slider = 25,
        //                 Type_Dial = 27,
        //                 Type_TextArea = 30,
        //                 Type_LineEdit = 31,
        //                 Type_Undefined = 40 };
        //
        std::cout << "view type: " + std::to_string(setting->viewType()) << std::endl;
        std::cout << "setting type: " + std::to_string(setting->type()) << std::endl;
        switch(setting->viewType())
        {
            case ViewType_Boolean:
                break;
            case ViewType_List:
                break;
            case ViewType_Range:
                break;
            case ViewType_Text:
                break;
            default:
                std::cerr << "ViewType_Undefined" << std::endl;
        }
                QLabel *lab = 0;
                if(!gridLayout->itemAtPosition(setting->viewRow(), 0))
                {
                    lab = new QLabel(section);
                    lab->setText(setting->name());
                    int textWidth = fm.width(setting->name()); // FIXME:
                    lab->setMinimumWidth(std::max(textWidth, this->width()/3));
                }
                else
                {
                    lab = dynamic_cast<QLabel*> (gridLayout->itemAtPosition(setting->viewRow(), 0)->widget());
                    QString oldLabel = lab->text();
                    lab->setText(oldLabel + ", " + setting->name());
                }
        switch(setting->type())
        {
            case CSMSettings::SettingType::Type_RadioButton:
            {
                int rowNum = gridLayout->rowCount();
                if(!gridLayout->itemAtPosition(setting->viewRow(), 0))
                    if(lab) gridLayout->addWidget(lab, rowNum, 0);
                //lab->setStyleSheet("background-color:yellow;");  // FIXME:
                int colNum = 1;
                if(setting->declaredValues().size() > 1) // button group FIXME: try isMultiValue()
                {
                    QButtonGroup * buttonGroup = new QButtonGroup(section);
                    //buttonGroup->setStyleSheet("background-color:red;");  // FIXME:
                    //gridLayout->addWidget(buttonGroup, rowNum, 1);
                    foreach(QString string, setting->declaredValues())
                    {
                        QRadioButton *rb = new QRadioButton(string, section);
                        //rb->setStyleSheet("background-color:blue;");  // FIXME:
                        gridLayout->addWidget(rb, rowNum, colNum++);
                        buttonGroup->addButton(rb);
                    }
                }
                else // single radio button
                {
                    std::cout << "default values size: " + std::to_string(setting->declaredValues().size()) << std::endl;
                    QRadioButton *rb = new QRadioButton(section);
                    if(rb) rb->setText(setting->declaredValues().at(0));
                    gridLayout->addWidget(rb, gridLayout->rowCount(), 1);
                }
                break;
            }
            case CSMSettings::SettingType::Type_LineEdit:
            {
                if(!gridLayout->itemAtPosition(setting->viewRow(), 0))
                    if(lab) gridLayout->addWidget(lab, setting->viewRow(), 0);
                QLineEdit *item = new QLineEdit(section);
                //QLabel *lab = new QLabel(section);
                //lab->setText(setting->name()); // FIXME: how to put lables in proper place if many items in the same row
                gridLayout->addWidget(item, setting->viewRow(), setting->viewColumn()+1,
                                            setting->rowSpan(), setting->columnSpan());
                item->setText(setting->defaultValues().at(0)); //FIXME: check if empty first?
                break;
            }
            case CSMSettings::SettingType::Type_ComboBox:
            {
                QComboBox *item = new QComboBox(section);
                //QLabel *lab = new QLabel(section);
                if(!gridLayout->itemAtPosition(setting->viewRow(), 0))
                    if(lab) gridLayout->addWidget(lab, setting->viewRow(), 0);
                //lab->setText(setting->name());
                gridLayout->addWidget(item, setting->viewRow(), setting->viewColumn()+1,
                                            setting->rowSpan(), setting->columnSpan());
                item->clear();
                item->addItems(setting->declaredValues()); //FIXME: check if empty first?
                break;
            }
            default:
                break;
        }
    }
    section->resize(section->width(), gridLayout->rowCount() * (fm.lineSpacing() + 10)); // 10 is extra line space
    //tab->adjustSize();
        //}
        //QWidget *item = new QWidget(tab);
        //item->setObjectName(QString::fromUtf8("cmbAntiAlias"));
    //}

    //new QListWidgetItem (pageName, mPageListWidget);
    //mPageListWidget->setFixedWidth (textWidth + 50);

    //mStackedWidget->addWidget (&dynamic_cast<QWidget &>(*(page)));
#endif
}

void CSVSettings::SettingWindow::createConnections
                                    (const QList <CSMSettings::Setting *> &list)
{
    foreach (const CSMSettings::Setting *setting, list)
    {
        View *masterView = findView (setting->page(), setting->name());

        CSMSettings::Connector *connector =
                                new CSMSettings::Connector (masterView, this);

        connect (masterView,
                 SIGNAL (viewUpdated(const QString &, const QStringList &)),
                 connector,
                 SLOT (slotUpdateSlaves())
                 );

        const CSMSettings::ProxyValueMap &proxyMap = setting->proxyLists();

        foreach (const QString &key, proxyMap.keys())
        {
            QStringList keyPair = key.split('/');

            if (keyPair.size() != 2)
                continue;

            View *slaveView = findView (keyPair.at(0), keyPair.at(1));

            if (!slaveView)
            {
                qWarning () << "Unable to create connection for view "
                            << key;
                continue;
            }

            QList <QStringList> proxyList = proxyMap.value (key);
            connector->addSlaveView (slaveView, proxyList);

            connect (slaveView,
                     SIGNAL (viewUpdated(const QString &, const QStringList &)),
                    connector,
                     SLOT (slotUpdateMaster()));
        }
    }
}

void CSVSettings::SettingWindow::setViewValues()
{
    //iterate each page and view, setting their definintions
    //if they exist in the model
    foreach (const Page *page, mPages)
    {
        foreach (const View *view, page->views())
        {
            //testing beforehand prevents overwriting a proxy setting
            if (!mModel->hasSettingDefinitions (view->viewKey()))
                continue;

            QStringList defs = mModel->definitions (view->viewKey());

            view->setSelectedValues(defs);
        }
    }
}

CSVSettings::View *CSVSettings::SettingWindow::findView
                            (const QString &pageName, const QString &setting)
{
    foreach (const Page *page, mPages)
    {
        if (page->objectName() == pageName)
            return page->findView (pageName, setting);
    }
    return 0;
}

void CSVSettings::SettingWindow::saveSettings()
{
    //setting the definition in the model automatically syncs with the file
    foreach (const Page *page, mPages)
    {
        foreach (const View *view, page->views())
        {
            if (!view->serializable())
                continue;

            mModel->setDefinitions (view->viewKey(), view->selectedValues());
        }
    }

    mModel->saveDefinitions();
}

void CSVSettings::SettingWindow::closeEvent (QCloseEvent *event)
{
    QApplication::focusWidget()->clearFocus();
}
