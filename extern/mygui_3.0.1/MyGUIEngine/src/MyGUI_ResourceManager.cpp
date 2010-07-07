/*!
	@file
	@author		Albert Semenov
	@date		09/2008
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
#include "MyGUI_ResourceManager.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_IResource.h"
#include "MyGUI_DataManager.h"
#include "MyGUI_FactoryManager.h"

#include "MyGUI_ResourceImageSet.h"

namespace MyGUI
{

	const std::string XML_TYPE("Resource");
	const std::string XML_TYPE_LIST("List");

	MYGUI_INSTANCE_IMPLEMENT( ResourceManager )

	void ResourceManager::initialise()
	{
		MYGUI_ASSERT(!mIsInitialise, INSTANCE_TYPE_NAME << " initialised twice");
		MYGUI_LOG(Info, "* Initialise: " << INSTANCE_TYPE_NAME);

		registerLoadXmlDelegate(XML_TYPE) = newDelegate(this, &ResourceManager::_load);
		registerLoadXmlDelegate(XML_TYPE_LIST) = newDelegate(this, &ResourceManager::_loadList);

		// регестрируем дефолтные ресурсы
		FactoryManager::getInstance().registerFactory<ResourceImageSet>(XML_TYPE);

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully initialized");
		mIsInitialise = true;
	}

	void ResourceManager::shutdown()
	{
		if (!mIsInitialise) return;
		MYGUI_LOG(Info, "* Shutdown: " << INSTANCE_TYPE_NAME);

		FactoryManager::getInstance().unregisterFactory<ResourceImageSet>(XML_TYPE);

		clear();
		unregisterLoadXmlDelegate(XML_TYPE);
		unregisterLoadXmlDelegate(XML_TYPE_LIST);

		mMapLoadXmlDelegate.clear();

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully shutdown");
		mIsInitialise = false;
	}

	bool ResourceManager::load(const std::string& _file)
	{
		return _loadImplement(_file, false, "", INSTANCE_TYPE_NAME);
	}

	void ResourceManager::_load(xml::ElementPtr _node, const std::string& _file, Version _version)
	{
		FactoryManager& factory = FactoryManager::getInstance();

		VectorGuid vector_guid;
		// берем детей и крутимся, основной цикл
		xml::ElementEnumerator root = _node->getElementEnumerator();
		while (root.next(XML_TYPE))
		{
			// парсим атрибуты
			std::string id, type, name;
			root->findAttribute("type", type);
			root->findAttribute("name", name);
			root->findAttribute("id", id);

			Guid guid(id);
			if (!guid.empty())
			{
				if (mResourcesID.find(guid) != mResourcesID.end())
				{
					MYGUI_LOG(Warning, "dublicate resource id " << guid.print());
				}
			}

			if (mResources.find(name) != mResources.end())
			{
				MYGUI_LOG(Warning, "dublicate resource name '" << name << "'");
			}

			vector_guid.push_back(guid);

			IObject* object = factory.createObject(XML_TYPE, type);
			if (object == nullptr)
			{
				MYGUI_LOG(Error, "resource type '" << type << "' not found");
				continue;
			}

			IResourcePtr resource = object->castType<IResource>();
			resource->deserialization(root.current(), _version);

			if (!guid.empty()) mResourcesID[guid] = resource;
			if (!name.empty()) mResources[name] = resource;
		}

		if (!vector_guid.empty())
		{
			mListFileGuid[_file] = vector_guid;
		}

	}

	std::string ResourceManager::getFileNameByID(const Guid& _id)
	{
		for (MapVectorString::iterator item=mListFileGuid.begin(); item!=mListFileGuid.end(); ++item)
		{
			for (VectorGuid::iterator item2=item->second.begin(); item2!=item->second.end(); ++item2)
			{
				if (*item2 == _id)
				{
					return item->first;
				}
			}
		}
		return "";
	}

	void ResourceManager::_loadList(xml::ElementPtr _node, const std::string& _file, Version _version)
	{
		// берем детей и крутимся, основной цикл
		xml::ElementEnumerator node = _node->getElementEnumerator();
		while (node.next(XML_TYPE_LIST))
		{
			std::string source;
			if (!node->findAttribute("file", source)) continue;
			MYGUI_LOG(Info, "Load ini file '" << source << "'");
			_loadImplement(source, false, "", INSTANCE_TYPE_NAME);
		}
	}

	ResourceManager::LoadXmlDelegate& ResourceManager::registerLoadXmlDelegate(const std::string& _key)
	{
		MapLoadXmlDelegate::iterator iter = mMapLoadXmlDelegate.find(_key);
		MYGUI_ASSERT(iter == mMapLoadXmlDelegate.end(), "name delegate is exist");
		return (mMapLoadXmlDelegate[_key] = LoadXmlDelegate());
	}

	void ResourceManager::unregisterLoadXmlDelegate(const std::string& _key)
	{
		MapLoadXmlDelegate::iterator iter = mMapLoadXmlDelegate.find(_key);
		if (iter != mMapLoadXmlDelegate.end()) mMapLoadXmlDelegate.erase(iter);
	}

	bool ResourceManager::_loadImplement(const std::string& _file, bool _match, const std::string& _type, const std::string& _instance)
	{
		IDataStream* data = DataManager::getInstance().getData(_file);
		if (data == nullptr)
		{
			MYGUI_LOG(Error, _instance << " : '" << _file << "', not found");
			return false;
		}

		xml::Document doc;
		if (!doc.open(data))
		{
			MYGUI_LOG(Error, _instance << " : '" << _file << "', " << doc.getLastError());

			// FIXME
			delete data;

			return false;
		}

		// FIXME
		delete data;

		xml::ElementPtr root = doc.getRoot();
		if ( (nullptr == root) || (root->getName() != "MyGUI") )
		{
			MYGUI_LOG(Error, _instance << " : '" << _file << "', tag 'MyGUI' not found");
			return false;
		}

		std::string type;
		if (root->findAttribute("type", type))
		{
			Version version = Version::parse(root->findAttribute("version"));
			MapLoadXmlDelegate::iterator iter = mMapLoadXmlDelegate.find(type);
			if (iter != mMapLoadXmlDelegate.end())
			{
				if ((!_match) || (type == _type)) (*iter).second(root, _file, version);
				else
				{
					MYGUI_LOG(Error, _instance << " : '" << _file << "', type '" << _type << "' not found");
					return false;
				}
			}
			else
			{
				MYGUI_LOG(Error, _instance << " : '" << _file << "', delegate for type '" << type << "'not found");
				return false;
			}
		}
		// предпологаем что будут вложенные
		else if (!_match)
		{
			xml::ElementEnumerator node = root->getElementEnumerator();
			while (node.next("MyGUI"))
			{
				if (node->findAttribute("type", type))
				{
					Version version = Version::parse(root->findAttribute("version"));
					MapLoadXmlDelegate::iterator iter = mMapLoadXmlDelegate.find(type);
					if (iter != mMapLoadXmlDelegate.end())
					{
						(*iter).second(node.current(), _file, version);
					}
					else
					{
						MYGUI_LOG(Error, _instance << " : '" << _file << "', delegate for type '" << type << "'not found");
					}
				}
				else
				{
					MYGUI_LOG(Error, _instance << " : '" << _file << "', tag 'type' not found");
				}
			}
		}

		return true;
	}

	IResourcePtr ResourceManager::getByID(const Guid& _id, bool _throw)
	{
		MapResourceID::iterator iter = mResourcesID.find(_id);
		if (iter == mResourcesID.end())
		{
			if (_throw) MYGUI_EXCEPT("resource '" << _id.print() << "' not found");
			MYGUI_LOG(Warning, "resource '" << _id.print() << "' not found");
			return nullptr;
		}
		return iter->second;
	}

	void ResourceManager::addResource(IResourcePtr _item)
	{
		if (!_item->getResourceName().empty())
			mResources[_item->getResourceName()] = _item;
		if (!_item->getResourceID().empty())
			mResourcesID[_item->getResourceID()] = _item;
	}

	void ResourceManager::removeResource(IResourcePtr _item)
	{
		if (_item == nullptr) return;

		if (!_item->getResourceName().empty())
		{
			MapResource::iterator item = mResources.find(_item->getResourceName());
			if (item != mResources.end())
				mResources.erase(item);
		}

		if (!_item->getResourceID().empty())
		{
			MapResourceID::iterator id = mResourcesID.find(_item->getResourceID());
			if (id != mResourcesID.end())
				mResourcesID.erase(id);
		}
	}

} // namespace MyGUI
