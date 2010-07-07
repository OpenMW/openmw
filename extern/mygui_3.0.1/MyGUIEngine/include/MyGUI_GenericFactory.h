/*!
    @file
    @author     Albert Semenov
    @date       06/2009
    @module
*/
#ifndef __MYGUI_GENERIC_FACTORY_H__
#define __MYGUI_GENERIC_FACTORY_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"
#include "MyGUI_Delegate.h"

namespace MyGUI
{

	template <typename Type>
	class GenericFactory
	{
	public:
		typedef delegates::CDelegate1<IObject*&> Delegate;
		static typename Delegate::IDelegate* getFactory()
		{
			return newDelegate(createFromFactory);
		}

	private:
		static void createFromFactory(IObject*& _instance)
		{
			_instance = new Type();
		}
	};

}

#endif // __MYGUI_GENERIC_FACTORY_H__
