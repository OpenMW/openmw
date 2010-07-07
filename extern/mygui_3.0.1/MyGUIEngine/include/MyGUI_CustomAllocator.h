/*!
	@file
	@author		Albert Semenov
	@date		05/2009
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
#ifndef __MYGUI_CUSTOM_ALLOCATOR_H__
#define __MYGUI_CUSTOM_ALLOCATOR_H__

#include <memory>
#include <limits>

// for Ogre version
#include <OgrePrerequisites.h>

#if OGRE_VERSION < MYGUI_DEFINE_VERSION(1, 6, 0)
#include <OgreMemoryManager.h>
#include <OgreNoMemoryMacros.h>
#endif

namespace MyGUI
{

	template<typename T>
	class Allocator
	{
	public:
		//    typedefs
		typedef T value_type;
		typedef value_type* pointer;
		typedef const value_type* const_pointer;
		typedef value_type& reference;
		typedef const value_type& const_reference;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	public:
		//    convert an allocator<T> to allocator<U>
		template<typename U>
		struct rebind
		{
			typedef Allocator<U> other;
		};

	public:
		inline explicit Allocator() { }
		inline ~Allocator() { }
		template<typename U>
		inline explicit Allocator(Allocator<U> const&) { }

		//    address
		inline pointer address(reference r) { return &r; }
		inline const_pointer address(const_reference r) { return &r; }

		//    memory allocation
		inline pointer allocate(size_type cnt, typename std::allocator<void>::const_pointer = 0)
		{
			return reinterpret_cast<pointer>(::operator new (cnt * sizeof (T)));
		}
		inline void deallocate(pointer p, size_type)
		{
			::operator delete (p);
		}

		//    size
		inline size_type max_size() const
		{
			return std::numeric_limits<size_type>::max() / sizeof(T);
		}

		//    construction/destruction
		inline void construct(pointer p, const T& t) { new (p) T(t); }
		inline void destroy(pointer p) { p->~T(); }

		inline bool operator==(Allocator const&) { return true; }
		inline bool operator!=(Allocator const& a) { return !operator==(a); }
	};

} // namespace MyGUI

#if OGRE_VERSION < MYGUI_DEFINE_VERSION(1, 6, 0)
#include <OgreMemoryMacros.h>
#endif

#endif // __MYGUI_CUSTOM_ALLOCATOR_H__
