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
#include "MyGUI_FactoryManager.h"
#include "MyGUI_FontManager.h"
#include "MyGUI_XmlDocument.h"

#include "MyGUI_ResourceManualFont.h"
#include "MyGUI_ResourceTrueTypeFont.h"

namespace MyGUI
{
	const std::string XML_TYPE("Font");
	const std::string XML_TYPE_RESOURCE("Resource");
	const std::string XML_TYPE_PROPERTY("Property");
	const std::string RESOURCE_DEFAULT_NAME("Default");

	MYGUI_INSTANCE_IMPLEMENT( FontManager )

	void FontManager::initialise()
	{
		MYGUI_ASSERT(!mIsInitialise, INSTANCE_TYPE_NAME << " initialised twice");
		MYGUI_LOG(Info, "* Initialise: " << INSTANCE_TYPE_NAME);

		ResourceManager::getInstance().registerLoadXmlDelegate(XML_TYPE) = newDelegate(this, &FontManager::_load);

		FactoryManager::getInstance().registerFactory<ResourceManualFont>(XML_TYPE_RESOURCE);
		FactoryManager::getInstance().registerFactory<ResourceTrueTypeFont>(XML_TYPE_RESOURCE);

		mDefaultName = "Default";

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully initialized");
		mIsInitialise = true;
	}

	void FontManager::shutdown()
	{
		if (!mIsInitialise) return;
		MYGUI_LOG(Info, "* Shutdown: " << INSTANCE_TYPE_NAME);

		MyGUI::ResourceManager::getInstance().unregisterLoadXmlDelegate(XML_TYPE);

		FactoryManager::getInstance().unregisterFactory<ResourceManualFont>(XML_TYPE_RESOURCE);
		FactoryManager::getInstance().unregisterFactory<ResourceTrueTypeFont>(XML_TYPE_RESOURCE);

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully shutdown");
		mIsInitialise = false;
	}

	bool FontManager::load(const std::string& _file)
	{
		return MyGUI::ResourceManager::getInstance()._loadImplement(_file, true, XML_TYPE, INSTANCE_TYPE_NAME);
	}

	void FontManager::_load(xml::ElementPtr _node, const std::string& _file, Version _version)
	{
		xml::ElementEnumerator font = _node->getElementEnumerator();
		while (font.next())
		{
			if (font->getName() == XML_TYPE)
			{
				std::string name;
				if (!font->findAttribute("name", name)) continue;

				std::string type;
				if (type.empty())
				{
					if (font->findAttribute("resolution").empty()) type = "ResourceManualFont";
					else type = "ResourceTrueTypeFont";
				}

				xml::Document doc;
				xml::ElementPtr root = doc.createRoot("MyGUI");
				xml::ElementPtr node = root->createChild("Resource");
				node->addAttribute("type", type);
				node->addAttribute("name", name);

				std::string tmp;
				if (font->findAttribute("source", tmp))
				{
					xml::ElementPtr prop = node->createChild("Property");
					prop->addAttribute("key", "Source");
					prop->addAttribute("value", tmp);
				}

				if (font->findAttribute("size", tmp))
				{
					xml::ElementPtr prop = node->createChild("Property");
					prop->addAttribute("key", "Size");
					prop->addAttribute("value", tmp);
				}

				if (font->findAttribute("resolution", tmp))
				{
					xml::ElementPtr prop = node->createChild("Property");
					prop->addAttribute("key", "Resolution");
					prop->addAttribute("value", tmp);
				}

				if (font->findAttribute("antialias_colour", tmp))
				{
					xml::ElementPtr prop = node->createChild("Property");
					prop->addAttribute("key", "Antialias");
					prop->addAttribute("value", tmp);
				}

				if (font->findAttribute("space_width", tmp))
				{
					xml::ElementPtr prop = node->createChild("Property");
					prop->addAttribute("key", "SpaceWidth");
					prop->addAttribute("value", tmp);
				}

				if (font->findAttribute("tab_width", tmp))
				{
					xml::ElementPtr prop = node->createChild("Property");
					prop->addAttribute("key", "TabWidth");
					prop->addAttribute("value", tmp);
				}

				if (font->findAttribute("cursor_width", tmp))
				{
					xml::ElementPtr prop = node->createChild("Property");
					prop->addAttribute("key", "CursorWidth");
					prop->addAttribute("value", tmp);
				}

				if (font->findAttribute("distance", tmp))
				{
					xml::ElementPtr prop = node->createChild("Property");
					prop->addAttribute("key", "Distance");
					prop->addAttribute("value", tmp);
				}

				if (font->findAttribute("offset_height", tmp))
				{
					xml::ElementPtr prop = node->createChild("Property");
					prop->addAttribute("key", "OffsetHeight");
					prop->addAttribute("value", tmp);
				}

				if (font->findAttribute("default_height", tmp))
				{
					xml::ElementPtr prop = node->createChild("Property");
					prop->addAttribute("key", "DefaultHeight");
					prop->addAttribute("value", tmp);
				}

				xml::ElementPtr codes = node->createChild("Codes");

				xml::ElementEnumerator codeold = font->getElementEnumerator();
				while (codeold.next("Code"))
				{
					xml::ElementPtr codenew = codes->createChild("Code");

					if (codeold->findAttribute("range", tmp))
						codenew->addAttribute("range", tmp);

					if (codeold->findAttribute("hide", tmp))
						codenew->addAttribute("hide", tmp);

					if (codeold->findAttribute("index", tmp))
						codenew->addAttribute("index", tmp);

					if (codeold->findAttribute("coord", tmp))
						codenew->addAttribute("coord", tmp);
				}

				ResourceManager::getInstance()._load(root, _file, _version);
			}
			else if (font->getName() == XML_TYPE_PROPERTY)
			{
				const std::string& key = font->findAttribute("key");
				const std::string& value = font->findAttribute("value");
				if (key == "Default")
					mDefaultName = value;
			}
		}
	}

	void FontManager::setDefaultFont(const std::string& _value)
	{
		mDefaultName = _value;
	}

	IFont* FontManager::getByName(const std::string& _name) const
	{
		IResource* result = nullptr;
		//FIXME הכÿ סמגלוסעטלמסעט רנטפע למזוע טלועü טלÿ Default
		if (!_name.empty() && _name != RESOURCE_DEFAULT_NAME)
			result = ResourceManager::getInstance().getByName(_name, false);

		if (result == nullptr)
			result = ResourceManager::getInstance().getByName(mDefaultName, false);

		return result ? result->castType<IFont>(false) : nullptr;
	}

} // namespace MyGUI
