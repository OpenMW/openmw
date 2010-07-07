/*!
	@file
	@author		Albert Semenov
	@date		06/2009
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
#ifndef __MYGUI_RESOURCE_HOLDER_H__
#define __MYGUI_RESOURCE_HOLDER_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Enumerator.h"

namespace MyGUI
{

	template <typename Type>
	class /*MYGUI_EXPORT */ResourceHolder
	{
	public:
		typedef std::map<std::string, Type*> MapResource;
		typedef Enumerator<MapResource> EnumeratorPtr;

		virtual ~ResourceHolder() { }
	public:
		/** Check is resource exist */
		bool isExist(const std::string& _name) const
		{
			return mResources.find(_name) != mResources.end();
		}

		/** Find resource by name*/
		Type* findByName(const std::string& _name) const
		{
			typename MapResource::const_iterator item = mResources.find(_name);
			return (item == mResources.end()) ? nullptr : item->second;
		}

		/** Get resource by name*/
		Type* getByName(const std::string& _name, bool _throw = true) const
		{
			Type* result = findByName(_name);
			MYGUI_ASSERT(result || !_throw, "Resource '" << _name << "' not found");
			return result;
		}

		bool remove(const std::string& _name)
		{
			typename MapResource::const_iterator item = mResources.find(_name);
			if (item != mResources.end())
			{
				delete item->second;
				mResources.erase(item->first);
				return true;
			}
			return false;
		}

		void clear()
		{
			for (typename MapResource::iterator item=mResources.begin(); item!=mResources.end(); ++item)
			{
				delete item->second;
			}
			mResources.clear();
		}

		EnumeratorPtr getEnumerator()
		{
			return EnumeratorPtr(mResources);
		}

		size_t getCount() const { return mResources.size(); }

	protected:
		MapResource mResources;
	};

} // namespace MyGUI

#endif // __MYGUI_RESOURCE_HOLDER_H__
