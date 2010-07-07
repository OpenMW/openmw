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
#ifndef __MYGUI_RESOURCE_MANAGER_H__
#define __MYGUI_RESOURCE_MANAGER_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Instance.h"
#include "MyGUI_Enumerator.h"
#include "MyGUI_Guid.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_IResource.h"
#include "MyGUI_ResourceHolder.h"
#include "MyGUI_Delegate.h"

namespace MyGUI
{

	class MYGUI_EXPORT ResourceManager :
		public ResourceHolder<IResource>
	{
		MYGUI_INSTANCE_HEADER( ResourceManager )

	public:
		void initialise();
		void shutdown();

	public:

		/** Load additional MyGUI *_resource.xml file */
		bool load(const std::string& _file);

		bool _loadImplement(const std::string& _file, bool _match, const std::string& _type, const std::string& _instance);
		void _load(xml::ElementPtr _node, const std::string& _file, Version _version);
		void _loadList(xml::ElementPtr _node, const std::string& _file, Version _version);

		/** Get resource by GUID */
		IResourcePtr getByID(const Guid& _id, bool _throw = true);

		std::string getFileNameByID(const Guid& _id);

		void addResource(IResourcePtr _item);

		void removeResource(IResourcePtr _item);

		typedef delegates::CDelegate3<xml::ElementPtr, const std::string &, Version> LoadXmlDelegate;

		LoadXmlDelegate& registerLoadXmlDelegate(const std::string& _key);

		void unregisterLoadXmlDelegate(const std::string& _key);

	/*obsolete:*/
#ifndef MYGUI_DONT_USE_OBSOLETE

		MYGUI_OBSOLETE("use : size_t ResourceManager::getCount()")
		size_t getResourceCount() { return getCount(); }

		MYGUI_OBSOLETE("use : IResourcePtr ResourceManager::getByName(const std::string& _name, bool _throw)")
		IResourcePtr getResource(const std::string& _name, bool _throw = true) { return getByName(_name, _throw); }

		MYGUI_OBSOLETE("use : IResourcePtr ResourceManager::getByID(const Guid& _id, bool _throw)")
		IResourcePtr getResource(const Guid& _id, bool _throw = true) { return getByID(_id, _throw); }

#endif // MYGUI_DONT_USE_OBSOLETE

	private:
		typedef std::map<Guid, IResourcePtr> MapResourceID;
		MapResourceID mResourcesID;

		// карта с делегатами для парсинга хмл блоков
		typedef std::map<std::string, LoadXmlDelegate> MapLoadXmlDelegate;
		MapLoadXmlDelegate mMapLoadXmlDelegate;

		std::string mResourceGroup;
		typedef std::vector<Guid> VectorGuid;
		typedef std::map<std::string, VectorGuid> MapVectorString;

		MapVectorString mListFileGuid;
	};

} // namespace MyGUI

#endif // __MYGUI_RESOURCE_MANAGER_H__
