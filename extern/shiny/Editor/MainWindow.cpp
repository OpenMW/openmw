#include "MainWindow.hpp"
#include "ui_mainwindow.h"

#include <iostream>

#include <QCloseEvent>
#include <QTimer>

#include <QInputDialog>
#include <QMessageBox>

#include "Editor.hpp"
#include "ColoredTabWidget.hpp"
#include "AddPropertyDialog.hpp"

sh::MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, mRequestShowWindow(false)
	, mRequestExit(false)
	, mIgnoreGlobalSettingChange(false)
	, mIgnoreConfigurationChange(false)
	, mIgnoreMaterialChange(false)
	, mIgnoreMaterialPropertyChange(false)
{
	ui->setupUi(this);

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(onIdle()));
	timer->start(50);

	QList<int> sizes;
	sizes << 250;
	sizes << 550;
	ui->splitter->setSizes(sizes);

	mMaterialModel = new QStringListModel(this);

	mMaterialProxyModel = new QSortFilterProxyModel(this);
	mMaterialProxyModel->setSourceModel(mMaterialModel);
	mMaterialProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	mMaterialProxyModel->setDynamicSortFilter(true);
	mMaterialProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

	ui->materialList->setModel(mMaterialProxyModel);
	ui->materialList->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->materialList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->materialList->setAlternatingRowColors(true);

	connect(ui->materialList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
			this,								SLOT(onMaterialSelectionChanged(QModelIndex,QModelIndex)));

	mMaterialPropertyModel = new QStandardItemModel(0, 2, this);
	mMaterialPropertyModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Name")));
	mMaterialPropertyModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Value")));
	connect(mMaterialPropertyModel,	SIGNAL(itemChanged(QStandardItem*)),
			this,					SLOT(onMaterialPropertyChanged(QStandardItem*)));

	mMaterialSortModel = new PropertySortModel(this);
	mMaterialSortModel->setSourceModel(mMaterialPropertyModel);
	mMaterialSortModel->setDynamicSortFilter(true);
	mMaterialSortModel->setSortCaseSensitivity(Qt::CaseInsensitive);

	ui->materialView->setModel(mMaterialSortModel);
	ui->materialView->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->materialView->setAlternatingRowColors(true);
	ui->materialView->setSortingEnabled(true);
	connect(ui->materialView,	SIGNAL(customContextMenuRequested(QPoint)),
				this,			SLOT(onContextMenuRequested(QPoint)));

	mGlobalSettingsModel = new QStandardItemModel(0, 2, this);
	mGlobalSettingsModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Name")));
	mGlobalSettingsModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Value")));
	connect(mGlobalSettingsModel,	SIGNAL(itemChanged(QStandardItem*)),
			this,					SLOT(onGlobalSettingChanged(QStandardItem*)));

	ui->globalSettingsView->setModel(mGlobalSettingsModel);
	ui->globalSettingsView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	ui->globalSettingsView->verticalHeader()->hide();
	ui->globalSettingsView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
	ui->globalSettingsView->setSelectionMode(QAbstractItemView::SingleSelection);

	ui->configurationList->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->configurationList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	connect(ui->configurationList,	SIGNAL(currentTextChanged(QString)),
							this,	SLOT(onConfigurationSelectionChanged(QString)));

	mConfigurationModel = new QStandardItemModel(0, 2, this);
	mConfigurationModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Name")));
	mConfigurationModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Value")));
	connect(mConfigurationModel,	SIGNAL(itemChanged(QStandardItem*)),
			this,					SLOT(onConfigurationChanged(QStandardItem*)));

	ui->configurationView->setModel(mConfigurationModel);
	ui->configurationView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	ui->configurationView->verticalHeader()->hide();
	ui->configurationView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
	ui->configurationView->setSelectionMode(QAbstractItemView::SingleSelection);
}

sh::MainWindow::~MainWindow()
{
	delete ui;
}

void sh::MainWindow::closeEvent(QCloseEvent *event)
{
	this->hide();
	event->ignore();
}

void sh::MainWindow::onIdle()
{
	if (mRequestShowWindow)
	{
		mRequestShowWindow = false;
		show();
	}

	if (mRequestExit)
	{
		QApplication::exit();
		return;
	}

	boost::mutex::scoped_lock lock(mSync->mUpdateMutex);


	mIgnoreMaterialChange = true;
	QString selected;

	QModelIndex selectedIndex = ui->materialList->selectionModel()->currentIndex();
	if (selectedIndex.isValid())
		selected = mMaterialModel->data(selectedIndex, Qt::DisplayRole).toString();

	QStringList list;

	for (std::vector<std::string>::const_iterator it = mState.mMaterialList.begin(); it != mState.mMaterialList.end(); ++it)
	{
		list.push_back(QString::fromStdString(*it));
	}

	if (mMaterialModel->stringList() != list)
	{
		mMaterialModel->setStringList(list);

		// quick hack to keep our selection when the model has changed
		if (!selected.isEmpty())
			for (int i=0; i<mMaterialModel->rowCount(); ++i)
			{
				const QModelIndex& index = mMaterialModel->index(i,0);
				if (mMaterialModel->data(index, Qt::DisplayRole).toString() == selected)
				{
					ui->materialList->setCurrentIndex(index);
					break;
				}
			}
	}
	mIgnoreMaterialChange = false;

	mIgnoreGlobalSettingChange = true;
	for (std::map<std::string, std::string>::const_iterator it = mState.mGlobalSettingsMap.begin();
		 it != mState.mGlobalSettingsMap.end(); ++it)
	{
		QList<QStandardItem *> list = mGlobalSettingsModel->findItems(QString::fromStdString(it->first));
		if (!list.empty()) // item was already there
		{
			// if it changed, set the value column
			if (mGlobalSettingsModel->data(mGlobalSettingsModel->index(list.front()->row(), 1)).toString()
					!= QString::fromStdString(it->second))
			{
				mGlobalSettingsModel->setItem(list.front()->row(), 1, new QStandardItem(QString::fromStdString(it->second)));
			}
		}
		else // item wasn't there; insert new row
		{
			QList<QStandardItem*> toAdd;
			QStandardItem* name = new QStandardItem(QString::fromStdString(it->first));
			name->setFlags(name->flags() &= ~Qt::ItemIsEditable);
			QStandardItem* value = new QStandardItem(QString::fromStdString(it->second));
			toAdd.push_back(name);
			toAdd.push_back(value);
			mGlobalSettingsModel->appendRow(toAdd);
		}
	}
	mIgnoreGlobalSettingChange = false;


	mIgnoreConfigurationChange = true;
	QList<QListWidgetItem*> selected_ = ui->configurationList->selectedItems();
	QString selectedStr;
	if (selected_.size())
		selectedStr = selected_.front()->text();

	ui->configurationList->clear();

	for (std::vector<std::string>::const_iterator it = mState.mConfigurationList.begin(); it != mState.mConfigurationList.end(); ++it)
		ui->configurationList->addItem(QString::fromStdString(*it));

	if (!selectedStr.isEmpty())
		for (int i=0; i<ui->configurationList->count(); ++i)
		{
			if (ui->configurationList->item(i)->text() == selectedStr)
			{
				ui->configurationList->setCurrentItem(ui->configurationList->item(i), QItemSelectionModel::ClearAndSelect);
			}
		}

	mIgnoreConfigurationChange = false;

	if (!mState.mErrors.empty())
	{
		ui->errorLog->append(QString::fromStdString(mState.mErrors));
		mState.mErrors = "";
		QColor color = ui->tabWidget->palette().color(QPalette::Normal, QPalette::Link);
		ui->tabWidget->tabBar()->setTabTextColor(3, color);
	}


	// process query results
	boost::mutex::scoped_lock lock2(mSync->mQueryMutex);
	for (std::vector<Query*>::iterator it = mQueries.begin(); it != mQueries.end();)
	{
		if ((*it)->mDone)
		{
			if (typeid(**it) == typeid(ConfigurationQuery))
				buildConfigurationModel(static_cast<ConfigurationQuery*>(*it));
			else if (typeid(**it) == typeid(MaterialQuery))
				buildMaterialModel(static_cast<MaterialQuery*>(*it));
			else if (typeid(**it) == typeid(MaterialPropertyQuery))
			{
				MaterialPropertyQuery* q = static_cast<MaterialPropertyQuery*>(*it);
				mIgnoreMaterialPropertyChange = true;
				if (getSelectedMaterial().toStdString() == q->mName)
				{
					for (int i=0; i<mMaterialPropertyModel->rowCount(); ++i)
					{
						if (mMaterialPropertyModel->item(i,0)->text() == QString::fromStdString(q->mPropertyName))
						{
							mMaterialPropertyModel->item(i,1)->setText(QString::fromStdString(q->mValue));
							if (mMaterialPropertyModel->item(i,1)->isCheckable())
								mMaterialPropertyModel->item(i,1)->setCheckState ((q->mValue == "true")
										? Qt::Checked : Qt::Unchecked);
						}
					}
				}
				mIgnoreMaterialPropertyChange = false;
			}

			delete *it;
			it = mQueries.erase(it);
		}
		else
			++it;
	}
}

void sh::MainWindow::onMaterialSelectionChanged (const QModelIndex & current, const QModelIndex & previous)
{
	if (mIgnoreMaterialChange)
		return;

	QString name = getSelectedMaterial();
	if (!name.isEmpty())
		requestQuery(new sh::MaterialQuery(name.toStdString()));
}

QString sh::MainWindow::getSelectedMaterial()
{
	QModelIndex selectedIndex = ui->materialList->selectionModel()->currentIndex();
	if (!selectedIndex.isValid())
		return QString("");

	return mMaterialProxyModel->data(selectedIndex, Qt::DisplayRole).toString();
}

void sh::MainWindow::onConfigurationSelectionChanged (const QString& current)
{
	if (mIgnoreConfigurationChange)
		return;
	requestQuery(new sh::ConfigurationQuery(current.toStdString()));
}

void sh::MainWindow::onGlobalSettingChanged(QStandardItem *item)
{
	if (mIgnoreGlobalSettingChange)
		return; // we are only interested in changes by the user, not by the backend.

	std::string name = mGlobalSettingsModel->data(mGlobalSettingsModel->index(item->row(), 0)).toString().toStdString();
	std::string value = mGlobalSettingsModel->data(mGlobalSettingsModel->index(item->row(), 1)).toString().toStdString();

	queueAction(new sh::ActionChangeGlobalSetting(name, value));
}

void sh::MainWindow::onConfigurationChanged (QStandardItem* item)
{
	QList<QListWidgetItem*> items = ui->configurationList->selectedItems();
	if (items.size())
	{
		std::string name = items.front()->text().toStdString();
		std::string key = mConfigurationModel->data(mConfigurationModel->index(item->row(), 0)).toString().toStdString();
		std::string value = mConfigurationModel->data(mConfigurationModel->index(item->row(), 1)).toString().toStdString();

		queueAction(new sh::ActionChangeConfiguration(name, key, value));

		requestQuery(new sh::ConfigurationQuery(name));
	}
}

void sh::MainWindow::on_lineEdit_textEdited(const QString &arg1)
{
	mMaterialProxyModel->setFilterFixedString(arg1);
}

void sh::MainWindow::on_actionSave_triggered()
{
	queueAction (new sh::ActionSaveAll());
}

void sh::MainWindow::on_actionNewMaterial_triggered()
{

}

void sh::MainWindow::on_actionDeleteMaterial_triggered()
{
	QModelIndex selectedIndex = ui->materialList->selectionModel()->currentIndex();
	QString name = mMaterialProxyModel->data(selectedIndex, Qt::DisplayRole).toString();

	queueAction (new sh::ActionDeleteMaterial(name.toStdString()));
}

void sh::MainWindow::queueAction(Action* action)
{
	boost::mutex::scoped_lock lock(mSync->mActionMutex);
	mActionQueue.push(action);
}

void sh::MainWindow::requestQuery(Query *query)
{
	boost::mutex::scoped_lock lock(mSync->mActionMutex);
	mQueries.push_back(query);
}

void sh::MainWindow::on_actionQuit_triggered()
{
	hide();
}

void sh::MainWindow::on_actionNewConfiguration_triggered()
{
	QInputDialog dialog(this);

	QString text = QInputDialog::getText(this, tr("New Configuration"),
											  tr("Configuration name:"));

	if (!text.isEmpty())
	{
		queueAction(new ActionCreateConfiguration(text.toStdString()));
	}
}

void sh::MainWindow::on_actionDeleteConfiguration_triggered()
{
	QList<QListWidgetItem*> items = ui->configurationList->selectedItems();
	if (items.size())
		queueAction(new ActionDeleteConfiguration(items.front()->text().toStdString()));
}

void sh::MainWindow::on_actionDeleteConfigurationProperty_triggered()
{
	QList<QListWidgetItem*> items = ui->configurationList->selectedItems();
	if (items.empty())
		return;
	std::string configurationName = items.front()->text().toStdString();

	QModelIndex current = ui->configurationView->currentIndex();
	if (!current.isValid())
		return;

	std::string propertyName = mConfigurationModel->data(mConfigurationModel->index(current.row(), 0)).toString().toStdString();

	queueAction(new sh::ActionDeleteConfigurationProperty(configurationName, propertyName));
	requestQuery(new sh::ConfigurationQuery(configurationName));
}

void sh::MainWindow::on_actionCloneMaterial_triggered()
{
	QModelIndex selectedIndex = ui->materialList->selectionModel()->currentIndex();
	QString name = mMaterialProxyModel->data(selectedIndex, Qt::DisplayRole).toString();
	if (name.isEmpty())
		return;

	QInputDialog dialog(this);

	QString text = QInputDialog::getText(this, tr("Clone material"),
											  tr("Name:"));

	if (!text.isEmpty())
	{
		queueAction(new ActionCloneMaterial(name.toStdString(), text.toStdString()));
	}
}

void sh::MainWindow::onContextMenuRequested(const QPoint &point)
{
	QPoint globalPos = ui->materialView->viewport()->mapToGlobal(point);

	QMenu menu;

	QList <QAction*> actions;
	actions.push_back(ui->actionNewProperty);
	actions.push_back(ui->actionDeleteProperty);
	actions.push_back(ui->actionCreatePass);
	actions.push_back(ui->actionCreateTextureUnit);
	menu.addActions(actions);

	menu.exec(globalPos);
}

void sh::MainWindow::getContext(QModelIndex index, int* passIndex, int* textureIndex, bool* isInPass, bool* isInTextureUnit)
{
	if (passIndex)
	{
		*passIndex = 0;
		if (isInPass)
			*isInPass = false;
		QModelIndex passModelIndex = index;
		// go up until we find the pass item.
		while (getPropertyKey(passModelIndex) != "pass" && passModelIndex.isValid())
			passModelIndex = passModelIndex.parent();

		if (passModelIndex.isValid())
		{
			if (passModelIndex.column() != 0)
				passModelIndex = passModelIndex.parent().child(passModelIndex.row(), 0);
			for (int i=0; i<mMaterialPropertyModel->rowCount(); ++i)
			{
				if (mMaterialPropertyModel->data(mMaterialPropertyModel->index(i, 0)).toString() == QString("pass"))
				{
					if (mMaterialPropertyModel->index(i, 0) == passModelIndex)
					{
						if (isInPass)
							*isInPass = true;
						break;
					}
					++(*passIndex);
				}
			}
		}
	}
	if (textureIndex)
	{
		*textureIndex = 0;
		if (isInTextureUnit)
			*isInTextureUnit = false;
		QModelIndex texModelIndex = index;
		// go up until we find the texture_unit item.
		while (getPropertyKey(texModelIndex) != "texture_unit" && texModelIndex.isValid())
			texModelIndex = texModelIndex.parent();
		if (texModelIndex.isValid())
		{
			if (texModelIndex.column() != 0)
				texModelIndex = texModelIndex.parent().child(texModelIndex.row(), 0);
			for (int i=0; i<mMaterialPropertyModel->rowCount(texModelIndex.parent()); ++i)
			{
				if (texModelIndex.parent().child(i, 0).data().toString() == QString("texture_unit"))
				{
					if (texModelIndex.parent().child(i, 0) == texModelIndex)
					{
						if (isInTextureUnit)
							*isInTextureUnit = true;
						break;
					}
					++(*textureIndex);
				}
			}
		}
	}
}

std::string sh::MainWindow::getPropertyKey(QModelIndex index)
{
	if (!index.parent().isValid())
		return mMaterialPropertyModel->data(mMaterialPropertyModel->index(index.row(), 0)).toString().toStdString();
	else
		return index.parent().child(index.row(), 0).data().toString().toStdString();
}

std::string sh::MainWindow::getPropertyValue(QModelIndex index)
{
	if (!index.parent().isValid())
		return mMaterialPropertyModel->data(mMaterialPropertyModel->index(index.row(), 1)).toString().toStdString();
	else
		return index.parent().child(index.row(), 1).data().toString().toStdString();
}

void sh::MainWindow::onMaterialPropertyChanged(QStandardItem *item)
{
	if (mIgnoreMaterialPropertyChange)
		return;

	QString material = getSelectedMaterial();
	if (material.isEmpty())
		return;

	// handle checkboxes being checked/unchecked
	std::string value = getPropertyValue(item->index());
	if (item->data(Qt::UserRole).toInt() == MaterialProperty::Boolean)
	{
		if (item->checkState() == Qt::Checked && value != "true")
			value = "true";
		else if (item->checkState() == Qt::Unchecked && value == "true")
			value = "false";
		item->setText(QString::fromStdString(value));
	}

	// handle inherited properties being changed, i.e. overridden by the current (derived) material
	if (item->data(Qt::UserRole+1).toInt() == MaterialProperty::Inherited_Unchanged)
	{
		QColor normalColor = ui->materialView->palette().color(QPalette::Normal, QPalette::WindowText);
		mIgnoreMaterialPropertyChange = true;
		mMaterialPropertyModel->item(item->index().row(), 0)
				->setData(QVariant(MaterialProperty::Inherited_Changed), Qt::UserRole+1);
		mMaterialPropertyModel->item(item->index().row(), 0)
				->setData(normalColor, Qt::ForegroundRole);
		mMaterialPropertyModel->item(item->index().row(), 1)
				->setData(QVariant(MaterialProperty::Inherited_Changed), Qt::UserRole+1);
		mMaterialPropertyModel->item(item->index().row(), 1)
				->setData(normalColor, Qt::ForegroundRole);
		mIgnoreMaterialPropertyChange = false;

		ui->materialView->scrollTo(mMaterialSortModel->mapFromSource(item->index()));
	}

	if (!item->index().parent().isValid())
	{
		// top level material property
		queueAction(new ActionSetMaterialProperty(
				material.toStdString(), getPropertyKey(item->index()), value));
	}
	else if (getPropertyKey(item->index()) == "texture_unit")
	{
		// texture unit name changed
		int passIndex, textureIndex;
		getContext(item->index(), &passIndex, &textureIndex);
		std::cout << "passIndex " << passIndex << " " << textureIndex << std::endl;

		queueAction(new ActionChangeTextureUnitName(
				material.toStdString(), passIndex, textureIndex, value));

	}
	else if (item->index().parent().data().toString() == "pass")
	{
		// pass property
		int passIndex;
		getContext(item->index(), &passIndex, NULL);
		/// \todo if shaders are changed, check that the material provides all properties needed by the shader
		queueAction(new ActionSetPassProperty(
				material.toStdString(), passIndex, getPropertyKey(item->index()), value));
	}
	else if (item->index().parent().data().toString() == "shader_properties")
	{
		// shader property
		int passIndex;
		getContext(item->index(), &passIndex, NULL);
		queueAction(new ActionSetShaderProperty(
				material.toStdString(), passIndex, getPropertyKey(item->index()), value));
	}
	else if (item->index().parent().data().toString() == "texture_unit")
	{
		// texture property
		int passIndex, textureIndex;
		getContext(item->index(), &passIndex, &textureIndex);
		queueAction(new ActionSetTextureProperty(
				material.toStdString(), passIndex, textureIndex, getPropertyKey(item->index()), value));
	}
}

void sh::MainWindow::buildMaterialModel(MaterialQuery *data)
{
	mMaterialPropertyModel->clear();

	mMaterialPropertyModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Name")));
	mMaterialPropertyModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Value")));

	for (std::map<std::string, MaterialProperty>::const_iterator it = data->mProperties.begin();
		 it != data->mProperties.end(); ++it)
	{
		addProperty(mMaterialPropertyModel->invisibleRootItem(), it->first, it->second);
	}

	for (std::vector<PassInfo>::iterator it = data->mPasses.begin();
		 it != data->mPasses.end(); ++it)
	{
		QStandardItem* passItem = new QStandardItem (QString("pass"));
		passItem->setFlags(passItem->flags() &= ~Qt::ItemIsEditable);
		passItem->setData(QVariant(static_cast<int>(MaterialProperty::Object)), Qt::UserRole);

		if (it->mShaderProperties.size())
		{
			QStandardItem* shaderPropertiesItem = new QStandardItem (QString("shader_properties"));
			shaderPropertiesItem->setFlags(shaderPropertiesItem->flags() &= ~Qt::ItemIsEditable);
			shaderPropertiesItem->setData(QVariant(static_cast<int>(MaterialProperty::Object)), Qt::UserRole);

			for (std::map<std::string, MaterialProperty>::iterator pit = it->mShaderProperties.begin();
				 pit != it->mShaderProperties.end(); ++pit)
			{
				addProperty(shaderPropertiesItem, pit->first, pit->second);
			}
			passItem->appendRow(shaderPropertiesItem);
		}

		for (std::map<std::string, MaterialProperty>::iterator pit = it->mProperties.begin();
			 pit != it->mProperties.end(); ++pit)
		{
			addProperty(passItem, pit->first, pit->second);
		}

		for (std::vector<TextureUnitInfo>::iterator tIt = it->mTextureUnits.begin();
			 tIt != it->mTextureUnits.end(); ++tIt)
		{
			QStandardItem* unitItem = new QStandardItem (QString("texture_unit"));
			unitItem->setFlags(unitItem->flags() &= ~Qt::ItemIsEditable);
			unitItem->setData(QVariant(static_cast<int>(MaterialProperty::Object)), Qt::UserRole);
			QStandardItem* nameItem = new QStandardItem (QString::fromStdString(tIt->mName));
			nameItem->setData(QVariant(static_cast<int>(MaterialProperty::Object)), Qt::UserRole);

			QList<QStandardItem*> texUnit;
			texUnit << unitItem << nameItem;

			for (std::map<std::string, MaterialProperty>::iterator pit = tIt->mProperties.begin();
				 pit != tIt->mProperties.end(); ++pit)
			{
				addProperty(unitItem, pit->first, pit->second);
			}

			passItem->appendRow(texUnit);
		}

		QList<QStandardItem*> toAdd;
		toAdd << passItem;
		toAdd << new QStandardItem(QString(""));
		mMaterialPropertyModel->appendRow(toAdd);
	}

	ui->materialView->expandAll();
	ui->materialView->resizeColumnToContents(0);
	ui->materialView->resizeColumnToContents(1);
}

void sh::MainWindow::addProperty(QStandardItem *parent, const std::string &key, MaterialProperty value, bool scrollTo)
{
	QList<QStandardItem*> toAdd;
	QStandardItem* keyItem = new QStandardItem(QString::fromStdString(key));
	keyItem->setFlags(keyItem->flags() &= ~Qt::ItemIsEditable);
	keyItem->setData(QVariant(value.mType), Qt::UserRole);
	keyItem->setData(QVariant(value.mSource), Qt::UserRole+1);
	toAdd.push_back(keyItem);

	QStandardItem* valueItem = NULL;
	if (value.mSource != MaterialProperty::None)
	{
		valueItem = new QStandardItem(QString::fromStdString(value.mValue));
		valueItem->setData(QVariant(value.mType), Qt::UserRole);
		valueItem->setData(QVariant(value.mSource), Qt::UserRole+1);
		toAdd.push_back(valueItem);
	}


	if (value.mSource == MaterialProperty::Inherited_Unchanged)
	{
		QColor color = ui->configurationView->palette().color(QPalette::Disabled, QPalette::WindowText);
		keyItem->setData(color, Qt::ForegroundRole);
		if (valueItem)
			valueItem->setData(color, Qt::ForegroundRole);
	}
	if (value.mType == MaterialProperty::Boolean && valueItem)
	{
		valueItem->setCheckable(true);
		valueItem->setCheckState((value.mValue == "true") ? Qt::Checked : Qt::Unchecked);
	}

	parent->appendRow(toAdd);

	if (scrollTo)
		ui->materialView->scrollTo(mMaterialSortModel->mapFromSource(keyItem->index()));
}

void sh::MainWindow::buildConfigurationModel(ConfigurationQuery *data)
{
	while (mConfigurationModel->rowCount())
		mConfigurationModel->removeRow(0);
	for (std::map<std::string, std::string>::iterator it = data->mProperties.begin();
		 it != data->mProperties.end(); ++it)
	{
		QList<QStandardItem*> toAdd;
		QStandardItem* name = new QStandardItem(QString::fromStdString(it->first));
		name->setFlags(name->flags() &= ~Qt::ItemIsEditable);
		QStandardItem* value = new QStandardItem(QString::fromStdString(it->second));
		toAdd.push_back(name);
		toAdd.push_back(value);
		mConfigurationModel->appendRow(toAdd);
	}

	// add items that are in global settings, but not in this configuration (with a "inactive" color)
	for (std::map<std::string, std::string>::const_iterator it = mState.mGlobalSettingsMap.begin();
		 it != mState.mGlobalSettingsMap.end(); ++it)
	{
		if (data->mProperties.find(it->first) == data->mProperties.end())
		{
			QColor color = ui->configurationView->palette().color(QPalette::Disabled, QPalette::WindowText);
			QList<QStandardItem*> toAdd;
			QStandardItem* name = new QStandardItem(QString::fromStdString(it->first));
			name->setFlags(name->flags() &= ~Qt::ItemIsEditable);
			name->setData(color, Qt::ForegroundRole);
			QStandardItem* value = new QStandardItem(QString::fromStdString(it->second));
			value->setData(color, Qt::ForegroundRole);
			toAdd.push_back(name);
			toAdd.push_back(value);
			mConfigurationModel->appendRow(toAdd);
		}
	}
}

void sh::MainWindow::on_actionCreatePass_triggered()
{
	QString material = getSelectedMaterial();
	if (!material.isEmpty())
	{
		addProperty(mMaterialPropertyModel->invisibleRootItem(),
					"pass", MaterialProperty("", MaterialProperty::Object, MaterialProperty::None), true);

		queueAction (new ActionCreatePass(material.toStdString()));
	}
}

void sh::MainWindow::on_actionDeleteProperty_triggered()
{
	QModelIndex selectedIndex = mMaterialSortModel->mapToSource(ui->materialView->selectionModel()->currentIndex());
	QString material = getSelectedMaterial();
	if (material.isEmpty())
		return;

	mIgnoreMaterialPropertyChange = true;

	if (getPropertyKey(selectedIndex) == "pass")
	{
		// delete whole pass
		int passIndex;
		getContext(selectedIndex, &passIndex, NULL);
		if (passIndex == 0)
		{
			QMessageBox msgBox;
			msgBox.setText("The first pass can not be deleted.");
			msgBox.exec();
		}
		else
		{
			queueAction(new ActionDeletePass(material.toStdString(), passIndex));
			mMaterialPropertyModel->removeRow(selectedIndex.row(), selectedIndex.parent());
		}
	}
	else if (getPropertyKey(selectedIndex) == "texture_unit")
	{
		// delete whole texture unit
		int passIndex, textureIndex;
		getContext(selectedIndex, &passIndex, &textureIndex);
		queueAction(new ActionDeleteTextureUnit(material.toStdString(), passIndex, textureIndex));
		mMaterialPropertyModel->removeRow(selectedIndex.row(), selectedIndex.parent());
	}
	else if (!selectedIndex.parent().isValid())
	{
		// top level material property
		MaterialProperty::Source source = static_cast<MaterialProperty::Source>(
					mMaterialPropertyModel->itemFromIndex(selectedIndex)->data(Qt::UserRole+1).toInt());
		if (source == MaterialProperty::Inherited_Unchanged)
		{
			QMessageBox msgBox;
			msgBox.setText("Inherited properties can not be deleted.");
			msgBox.exec();
		}
		else
		{
			queueAction(new ActionDeleteMaterialProperty(
					material.toStdString(), getPropertyKey(selectedIndex)));
			std::cout << "source is " << source << std::endl;
			if (source == MaterialProperty::Inherited_Changed)
			{
				QColor inactiveColor = ui->materialView->palette().color(QPalette::Disabled, QPalette::WindowText);
				mMaterialPropertyModel->item(selectedIndex.row(), 0)
						->setData(QVariant(MaterialProperty::Inherited_Unchanged), Qt::UserRole+1);
				mMaterialPropertyModel->item(selectedIndex.row(), 0)
						->setData(inactiveColor, Qt::ForegroundRole);
				mMaterialPropertyModel->item(selectedIndex.row(), 1)
						->setData(QVariant(MaterialProperty::Inherited_Unchanged), Qt::UserRole+1);
				mMaterialPropertyModel->item(selectedIndex.row(), 1)
						->setData(inactiveColor, Qt::ForegroundRole);

				// make sure to update the property's value
				requestQuery(new sh::MaterialPropertyQuery(material.toStdString(), getPropertyKey(selectedIndex)));
			}
			else
				mMaterialPropertyModel->removeRow(selectedIndex.row());
		}
	}
	else if (selectedIndex.parent().data().toString() == "pass")
	{
		// pass property
		int passIndex;
		getContext(selectedIndex, &passIndex, NULL);
		queueAction(new ActionDeletePassProperty(
				material.toStdString(), passIndex, getPropertyKey(selectedIndex)));
		mMaterialPropertyModel->removeRow(selectedIndex.row(), selectedIndex.parent());
	}
	else if (selectedIndex.parent().data().toString() == "shader_properties")
	{
		// shader property
		int passIndex;
		getContext(selectedIndex, &passIndex, NULL);
		queueAction(new ActionDeleteShaderProperty(
				material.toStdString(), passIndex, getPropertyKey(selectedIndex)));
		mMaterialPropertyModel->removeRow(selectedIndex.row(), selectedIndex.parent());
	}
	else if (selectedIndex.parent().data().toString() == "texture_unit")
	{
		// texture property
		int passIndex, textureIndex;
		getContext(selectedIndex, &passIndex, &textureIndex);
		queueAction(new ActionDeleteTextureProperty(
				material.toStdString(), passIndex, textureIndex, getPropertyKey(selectedIndex)));
		mMaterialPropertyModel->removeRow(selectedIndex.row(), selectedIndex.parent());
	}
	mIgnoreMaterialPropertyChange = false;
}

void sh::MainWindow::on_actionNewProperty_triggered()
{
	QModelIndex selectedIndex = mMaterialSortModel->mapToSource(ui->materialView->selectionModel()->currentIndex());
	QString material = getSelectedMaterial();
	if (material.isEmpty())
		return;

	AddPropertyDialog* dialog = new AddPropertyDialog(this);
	dialog->exec();
	QString propertyName = dialog->mName;
	QString defaultValue = "";

	/// \todo check if this property name exists already

	if (!propertyName.isEmpty())
	{
		int passIndex, textureIndex;
		bool isInPass, isInTextureUnit;
		getContext(selectedIndex, &passIndex, &textureIndex, &isInPass, &isInTextureUnit);

		QList<QStandardItem*> items;
		QStandardItem* keyItem = new QStandardItem(propertyName);
		keyItem->setFlags(keyItem->flags() &= ~Qt::ItemIsEditable);
		items << keyItem;
		items << new QStandardItem(defaultValue);

		// figure out which item the new property should be a child of
		QModelIndex parentIndex = selectedIndex;
		if (selectedIndex.data(Qt::UserRole) != MaterialProperty::Object)
			parentIndex = selectedIndex.parent();
		QStandardItem* parentItem;
		if (!parentIndex.isValid())
			parentItem = mMaterialPropertyModel->invisibleRootItem();
		else
			parentItem = mMaterialPropertyModel->itemFromIndex(parentIndex);

		if (isInTextureUnit)
		{
			queueAction(new ActionSetTextureProperty(
							material.toStdString(), passIndex, textureIndex, propertyName.toStdString(), defaultValue.toStdString()));
		}
		else if (isInPass)
		{
			if (selectedIndex.parent().child(selectedIndex.row(),0).data().toString() == "shader_properties"
				|| selectedIndex.parent().data().toString() == "shader_properties")
			{
				queueAction(new ActionSetShaderProperty(
								material.toStdString(), passIndex, propertyName.toStdString(), defaultValue.toStdString()));
			}
			else
			{
				queueAction(new ActionSetPassProperty(
								material.toStdString(), passIndex, propertyName.toStdString(), defaultValue.toStdString()));
			}
		}
		else
		{
			queueAction(new ActionSetMaterialProperty(
							material.toStdString(), propertyName.toStdString(), defaultValue.toStdString()));
		}

		addProperty(parentItem, propertyName.toStdString(),
					MaterialProperty (defaultValue.toStdString(), MaterialProperty::Misc, MaterialProperty::Normal), true);

		/// \todo scroll to newly added property
	}
}

void sh::MainWindow::on_actionCreateTextureUnit_triggered()
{
	QString material = getSelectedMaterial();
	if (material.isEmpty())
		return;

	QInputDialog dialog(this);

	QString text = QInputDialog::getText(this, tr("New texture unit"),
											  tr("Texture unit name (for referencing in shaders):"));
	if (!text.isEmpty())
	{
		QModelIndex selectedIndex = mMaterialSortModel->mapToSource(ui->materialView->selectionModel()->currentIndex());
		int passIndex;
		getContext(selectedIndex, &passIndex, NULL);
		queueAction(new ActionCreateTextureUnit(material.toStdString(), passIndex, text.toStdString()));

		// add to model
		int index = 0;
		for (int i=0; i<mMaterialPropertyModel->rowCount(); ++i)
		{
			if (mMaterialPropertyModel->data(mMaterialPropertyModel->index(i, 0)).toString() == QString("pass"))
			{
				if (index == passIndex)
				{
					addProperty(mMaterialPropertyModel->itemFromIndex(mMaterialPropertyModel->index(i, 0)),
								"texture_unit", MaterialProperty(text.toStdString(), MaterialProperty::Object), true);
					break;
				}

				++index;
			}
		}
	}
}

void sh::MainWindow::on_clearButton_clicked()
{
	ui->errorLog->clear();
}

void sh::MainWindow::on_tabWidget_currentChanged(int index)
{
	QColor color = ui->tabWidget->palette().color(QPalette::Normal, QPalette::WindowText);

	if (index == 3)
		ui->tabWidget->tabBar()->setTabTextColor(3, color);
}
