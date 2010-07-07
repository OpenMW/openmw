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
#ifndef __MYGUI_BITWISE_H__
#define __MYGUI_BITWISE_H__

#include "MyGUI_Prerequest.h"

namespace MyGUI
{

	class Bitwise
	{
	public:
		/** Returns the closest power-of-two number greater or equal to value.
		*/
		template<typename Type>
		static MYGUI_FORCEINLINE Type firstPO2From(Type _value)
		{
			--_value;
			_value |= _value >> 16;
			_value |= _value >> 8;
			_value |= _value >> 4;
			_value |= _value >> 2;
			_value |= _value >> 1;
			++_value;
			return _value;
		}

		/** Determines whether the number is power-of-two or not. */
		template<typename Type>
		static MYGUI_FORCEINLINE bool isPO2(Type _value)
		{
			return (_value & (_value-1)) == 0;
		}

		/** Returns the number of bits a pattern must be shifted right by to
		remove right-hand zeros.
		*/
		template<typename Type>
		static MYGUI_FORCEINLINE size_t getBitShift(Type _mask)
		{
			if (_mask == 0)
				return 0;

			size_t result = 0;
			while ((_mask & 1) == 0)
			{
				++result;
				_mask >>= 1;
			}
			return result;
		}

	};

} // namespace MyGUI

#endif // __MYGUI_BITWISE_H__
