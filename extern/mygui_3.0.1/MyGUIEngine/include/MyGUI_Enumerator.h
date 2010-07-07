/*!
	@file
	@author		Albert Semenov
	@date		08/2008
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
#ifndef __MYGUI_ENUMERATOR_H__
#define __MYGUI_ENUMERATOR_H__

#include <assert.h>

namespace MyGUI
{

	/** Class for comfortable using of vectors with small while loop
	instead iterators. Performance is same as with iterators.
	Enumerator usage
	@example Enumerator
	@code
		typedef std::vector<std::string> VectorString;
		typedef Enumerator<VectorString> EnumeratorVectorString;

		VectorString vec;
		vec.push_back("value");
		//EnumeratorVectorString enum_vec(vec.begin(), vec.end());
		EnumeratorVectorString enum_vec(vec);
		while (enum_vec.next())
		{
			std::string value = enum_vec.current();
		}

		typedef std::pair<std::string, std::string> PairString;
		typedef std::map<PairString> MapString;

		MapString map;
		map["key"] = "value";
		//EnumeratorMapString enum_map(map.begin(), map.end());
		EnumeratorMapString enum_map(map);
		while (enum_map.next())
		{
			std::string key = enum_map.current().first;
			std::string value = enum_map.current().second;
		}
	@endcode
	*/

	template<typename T>
	class Enumerator
	{
	private:
		Enumerator() { }

	public:
		explicit Enumerator(const T& _container) :
			m_first(true),
			m_current(_container.begin()),
			m_end(_container.end())
		{
		}

		Enumerator(typename T::const_iterator _first, typename T::const_iterator _end) :
			m_first(true),
			m_current(_first),
			m_end(_end)
		{
		}

		bool next()
		{
			if (m_current == m_end) return false;
			else if (m_first)
			{
				m_first = false;
				return true;
			}
			++ m_current;
			if (m_current == m_end) return false;
			return true;
		}

		typename T::const_reference operator->() const { assert(m_current != m_end); return (*m_current); }
		typename T::const_reference current() { assert(m_current != m_end); return (*m_current); }

	private:
		bool m_first;
		typename T::const_iterator m_current;
		typename T::const_iterator m_end;
	};

} // namespace MyGUI

#endif // __MYGUI_ENUMERATOR_H__
