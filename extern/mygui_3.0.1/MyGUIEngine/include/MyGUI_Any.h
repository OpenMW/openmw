/*!
	@file
	@author		Albert Semenov
	@date		10/2008
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

// -- Based on boost::any, original copyright information follows --
// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0.
// (See at http://www.boost.org/LICENSE_1_0.txt)
// -- End original copyright --

#ifndef __MYGUI_ANY_H__
#define __MYGUI_ANY_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Diagnostic.h"
#include <algorithm>
#include <typeinfo>

namespace MyGUI
{

	/** @example "Class Any usage"
	@code
	void f()
	{
		// RU: тестовый класс, с простыми типами все аналогично
		// EN: test class, with simple types all is similar
		struct Data { int value; };

		// RU: экземпляр и инициализация
		// EN: instance and initialization
		Data data;
		data.value = 0xDEAD;

		// RU: создастся копия класса Data
		// EN: copy of class Data will be created
		MyGUI::Any any = data;
		// RU: копия класса Data
		// EN: copy of class Data
		Data copy_data = *any.castType<Data>();
		// RU: теперь value == 0xDEAD
		// EN: now value == 0xDEAD
		int value = copy_data.value;


		// RU: создастся копия указателя на класс Data
		// EN: copy of pointer on class Data will be created
		any = &data;
		// RU: копия указателя на класс Data и конкретно на объект data
		// EN: copy of pointer on class Data and on object data
		Data * copy_ptr = *any.castType<Data*>();
		// RU: теперь data.value == 0
		// EN: now value == 0
		copy_ptr->value = 0;

	}
	@endcode
	*/

	class MYGUI_EXPORT Any
	{

	private:
		struct AnyEmpty { };

	public:
		static AnyEmpty Null;

	public:
		Any() :
			mContent(nullptr)
		{
		}

		template<typename ValueType> Any(const ValueType& value) :
			mContent(new Holder<ValueType>(value))
		{
		}

		Any(const Any::AnyEmpty& value) :
			mContent(nullptr)
		{
		}

		Any(const Any& other) :
			mContent(other.mContent ? other.mContent->clone() : nullptr)
		{
		}

		~Any()
		{
			delete mContent;
		}

		Any& swap(Any& rhs)
		{
			std::swap(mContent, rhs.mContent);
			return *this;
		}

		template<typename ValueType> Any& operator = (const ValueType& rhs)
		{
			Any(rhs).swap(*this);
			return *this;
		}

		Any& operator = (const Any::AnyEmpty& rhs)
		{
			delete mContent;
			mContent = nullptr;
			return *this;
		}

		Any& operator = (const Any& rhs)
		{
			Any(rhs).swap(*this);
			return *this;
		}

		bool empty() const
		{
			return !mContent;
		}

		const std::type_info& getType() const
		{
			return mContent ? mContent->getType() : typeid(void);
		}

		template<typename ValueType>
		ValueType * castType(bool _throw = true) const
		{
			if (this->getType() == typeid(ValueType))
			{
				return &static_cast<Any::Holder<ValueType> *>(this->mContent)->held;
			}
			MYGUI_ASSERT(!_throw, "Bad cast from type '" << getType().name() << "' to '" << typeid(ValueType).name() << "'");
			return nullptr;
		}

		void * castUnsafe() const
		{
			return mContent ? static_cast<Any::Holder<void *> *>(this->mContent)->held : nullptr;
		}

	private:
		class Placeholder
		{
		public:
			virtual ~Placeholder() { }

		public:
			virtual const std::type_info& getType() const = 0;
			virtual Placeholder * clone() const = 0;

		};

		template<typename ValueType> class Holder : public Placeholder
		{
		public:
			Holder(const ValueType& value) :
				held(value)
			{
			}

		public:
			virtual const std::type_info& getType() const
			{
				return typeid(ValueType);
			}

			virtual Placeholder * clone() const
			{
				return new Holder(held);
			}

		public:
			ValueType held;

		private:
			Holder& operator=(const Holder &);

		};


		private: // representation
			Placeholder * mContent;

	};

} // namespace MyGUI

#endif // __MYGUI_ANY_H__
