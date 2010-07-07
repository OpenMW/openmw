/*!
	@file
	@author		Albert Semenov
	@date		05/2008
	@module
*/

#ifndef __MYGUI_OGRE_DATA_MANAGER_H__
#define __MYGUI_OGRE_DATA_MANAGER_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Instance.h"
#include "MyGUI_DataManager.h"

namespace MyGUI
{

	class OgreDataManager :
		public DataManager
	{
		MYGUI_INSTANCE_HEADER(OgreDataManager)

	public:
		void initialise(const std::string& _group);
		void shutdown();

		const std::string& getGroup() { return mGroup; }

		virtual IDataStream* getData(const std::string& _name);

		typedef std::vector<std::string> VectorString;

		virtual bool isDataExist(const std::string& _name);

		virtual const VectorString& getDataListNames(const std::string& _pattern);

		virtual const std::string& getDataPath(const std::string& _name);

	private:
		const VectorString& getDataListNames(const std::string& _pattern, bool _fullpath);

	private:
		std::string mGroup;

	};

} // namespace MyGUI

#endif // __MYGUI_OGRE_DATA_MANAGER_H__
