/*!
	@file
	@author		Albert Semenov
	@date		11/2007
	@module
*/
/*
	This file is part of MyGUI.

	MyGUI is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MyGUI is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with MyGUI.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "MyGUI_Precompiled.h"
#include "MyGUI_SkinManager.h"
#include "MyGUI_LanguageManager.h"
#include "MyGUI_ResourceSkin.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_SubWidgetManager.h"
#include "MyGUI_Gui.h"
#include "MyGUI_DataManager.h"
#include "MyGUI_FactoryManager.h"
#include "MyGUI_IStateInfo.h"

namespace MyGUI
{

	const std::string XML_TYPE("Skin");
	const std::string XML_TYPE_RESOURCE("Resource");
	const std::string RESOURCE_DEFAULT_NAME("Default");

	MYGUI_INSTANCE_IMPLEMENT( SkinManager )

	void SkinManager::initialise()
	{
		MYGUI_ASSERT(!mIsInitialise, INSTANCE_TYPE_NAME << " initialised twice");
		MYGUI_LOG(Info, "* Initialise: " << INSTANCE_TYPE_NAME);

		ResourceManager::getInstance().registerLoadXmlDelegate(XML_TYPE) = newDelegate(this, &SkinManager::_load);
		FactoryManager::getInstance().registerFactory<ResourceSkin>(XML_TYPE_RESOURCE);

		mDefaultName = "skin_Default";
		createDefault(mDefaultName);

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully initialized");
		mIsInitialise = true;
	}

	void SkinManager::shutdown()
	{
		if (!mIsInitialise) return;
		MYGUI_LOG(Info, "* Shutdown: " << INSTANCE_TYPE_NAME);

		ResourceManager::getInstance().unregisterLoadXmlDelegate(XML_TYPE);
		FactoryManager::getInstance().unregisterFactory<ResourceSkin>(XML_TYPE_RESOURCE);

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully shutdown");
		mIsInitialise = false;
	}

	bool SkinManager::load(const std::string& _file)
	{
		return ResourceManager::getInstance()._loadImplement(_file, true, XML_TYPE, INSTANCE_TYPE_NAME);
	}

	void SkinManager::_load(xml::ElementPtr _node, const std::string& _file, Version _version)
	{
		// берем детей и крутимся, основной цикл со скинами
		xml::ElementEnumerator skin = _node->getElementEnumerator();
		while (skin.next(XML_TYPE))
		{
			std::string name = skin->findAttribute("name");
			std::string type = skin->findAttribute("type");
			if (type.empty()) type = "ResourceSkin";

			IObject* object = FactoryManager::getInstance().createObject(XML_TYPE_RESOURCE, type);
			if (object != nullptr)
			{
				ResourceSkin* data = object->castType<ResourceSkin>();
				data->deserialization(skin.current(), _version);

				ResourceManager::getInstance().addResource(data);
			}
		}
	}

	void SkinManager::createDefault(const std::string& _value)
	{
		xml::Document doc;
		xml::ElementPtr root = doc.createRoot("MyGUI");
		xml::ElementPtr newnode = root->createChild("Resource");
		newnode->addAttribute("type", ResourceSkin::getClassTypeName());
		newnode->addAttribute("name", _value);

		ResourceManager::getInstance()._load(root, "", Version());
	}

	ResourceSkin* SkinManager::getByName(const std::string& _name) const
	{
		IResource* result = nullptr;
		if (!_name.empty() && _name != RESOURCE_DEFAULT_NAME)
			result = ResourceManager::getInstance().getByName(_name, false);

		if (result == nullptr)
		{
			result = ResourceManager::getInstance().getByName(mDefaultName, false);
			MYGUI_LOG(Error, "Skin '" << _name << "' not found. Replaced with default skin.");
		}

		return result ? result->castType<ResourceSkin>(false) : nullptr;
	}

	bool SkinManager::isExist(const std::string& _name) const
	{
		return ResourceManager::getInstance().isExist(_name);
	}

	void SkinManager::setDefaultSkin(const std::string& _value)
	{
		mDefaultName = _value;
	}

} // namespace MyGUI
