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

namespace delegates
{

	#define MYGUI_COMBINE(a, b)									MYGUI_COMBINE1(a, b)
	#define MYGUI_COMBINE1(a, b)								a##b

	#define MYGUI_I_DELEGATE									MYGUI_COMBINE(IDelegate, MYGUI_SUFFIX)

	#define MYGUI_C_STATIC_DELEGATE					MYGUI_COMBINE(CStaticDelegate, MYGUI_SUFFIX)
	#define MYGUI_C_METHOD_DELEGATE					MYGUI_COMBINE(CMethodDelegate, MYGUI_SUFFIX)

	#define MYGUI_C_DELEGATE									MYGUI_COMBINE(CDelegate, MYGUI_SUFFIX)
	#define MYGUI_C_MULTI_DELEGATE						MYGUI_COMBINE(CMultiDelegate, MYGUI_SUFFIX)


	// базовый класс всех делегатов
	MYGUI_TEMPLATE   MYGUI_TEMPLATE_PARAMS
	class MYGUI_I_DELEGATE
	{
	public:
		virtual ~MYGUI_I_DELEGATE() { }
		virtual bool isType( const std::type_info& _type) = 0;
		virtual void invoke( MYGUI_PARAMS ) = 0;
		virtual bool compare(  MYGUI_I_DELEGATE MYGUI_TEMPLATE_ARGS  * _delegate) const = 0;
		virtual bool compare(IDelegateUnlink * _unlink) const { return false; }
	};


	// делегат для статической функции
	MYGUI_TEMPLATE   MYGUI_TEMPLATE_PARAMS
	class MYGUI_C_STATIC_DELEGATE : public  MYGUI_I_DELEGATE MYGUI_TEMPLATE_ARGS
	{
	public:
		typedef void (*Func)( MYGUI_PARAMS );

		MYGUI_C_STATIC_DELEGATE (Func _func) : mFunc(_func) { }

		virtual bool isType( const std::type_info& _type) { return typeid( MYGUI_C_STATIC_DELEGATE MYGUI_TEMPLATE_ARGS ) == _type; }

		virtual void invoke( MYGUI_PARAMS )
		{
			mFunc( MYGUI_ARGS );
		}

		virtual bool compare(  MYGUI_I_DELEGATE MYGUI_TEMPLATE_ARGS  * _delegate) const
		{
			if (nullptr == _delegate || !_delegate->isType(typeid(MYGUI_C_STATIC_DELEGATE MYGUI_TEMPLATE_ARGS)) ) return false;
			MYGUI_C_STATIC_DELEGATE MYGUI_TEMPLATE_ARGS * cast = static_cast<MYGUI_C_STATIC_DELEGATE MYGUI_TEMPLATE_ARGS *>(_delegate);
			return cast->mFunc == mFunc;
		}
		virtual bool compare(IDelegateUnlink * _unlink) const { return false; }

	private:
		Func mFunc;
	};


	// делегат для метода класса
	template MYGUI_T_TEMPLATE_PARAMS
	class MYGUI_C_METHOD_DELEGATE : public  MYGUI_I_DELEGATE MYGUI_TEMPLATE_ARGS
	{
	public:
		typedef void (T::*Method)( MYGUI_PARAMS );

		MYGUI_C_METHOD_DELEGATE(IDelegateUnlink * _unlink, T * _object, Method _method) : mUnlink(_unlink), mObject(_object), mMethod(_method) { }

		virtual bool isType( const std::type_info& _type) { return typeid( MYGUI_C_METHOD_DELEGATE MYGUI_T_TEMPLATE_ARGS ) == _type; }

		virtual void invoke( MYGUI_PARAMS )
		{
			(mObject->*mMethod)( MYGUI_ARGS );
		}

		virtual bool compare(  MYGUI_I_DELEGATE MYGUI_TEMPLATE_ARGS  * _delegate) const
		{
			if (nullptr == _delegate || !_delegate->isType(typeid(MYGUI_C_METHOD_DELEGATE MYGUI_T_TEMPLATE_ARGS)) ) return false;
			MYGUI_C_METHOD_DELEGATE MYGUI_T_TEMPLATE_ARGS  * cast = static_cast<  MYGUI_C_METHOD_DELEGATE MYGUI_T_TEMPLATE_ARGS  * >(_delegate);
			return cast->mObject == mObject && cast->mMethod == mMethod;
		}

		virtual bool compare(IDelegateUnlink * _unlink) const
		{
			return mUnlink == _unlink;
		}

	private:
		IDelegateUnlink *mUnlink;
		T * mObject;
		Method mMethod;
	};

} // namespace delegates

// шаблон для создания делегата статической функции
// параметры : указатель на функцию
// пример : newDelegate(funk_name);
// пример : newDelegate(class_name::static_method_name);
MYGUI_TEMPLATE   MYGUI_TEMPLATE_PARAMS
inline  delegates::MYGUI_I_DELEGATE MYGUI_TEMPLATE_ARGS  * newDelegate( void (*_func)( MYGUI_PARAMS ) )
{
	return new delegates::MYGUI_C_STATIC_DELEGATE MYGUI_TEMPLATE_ARGS  (_func);
}


// шаблон для создания делегата метода класса
// параметры : указатель на объект класса и указатель на метод класса
// пример : newDelegate(&object_name, &class_name::method_name);
template MYGUI_T_TEMPLATE_PARAMS
inline  delegates::MYGUI_I_DELEGATE MYGUI_TEMPLATE_ARGS  * newDelegate( T * _object, void (T::*_method)( MYGUI_PARAMS ) )
{
	return new delegates::MYGUI_C_METHOD_DELEGATE  MYGUI_T_TEMPLATE_ARGS  (delegates::GetDelegateUnlink(_object), _object, _method);
}

namespace delegates
{
	// шаблон класса делегата
	MYGUI_TEMPLATE   MYGUI_TEMPLATE_PARAMS
	class MYGUI_C_DELEGATE
	{
	public:
		typedef  MYGUI_I_DELEGATE MYGUI_TEMPLATE_ARGS  IDelegate;

		MYGUI_C_DELEGATE () : mDelegate(nullptr) { }
		MYGUI_C_DELEGATE (const MYGUI_C_DELEGATE  MYGUI_TEMPLATE_ARGS& _event)
		{
			// забираем себе владение
			mDelegate = _event.mDelegate;
			const_cast< MYGUI_C_DELEGATE  MYGUI_TEMPLATE_ARGS& >(_event).mDelegate = nullptr;
		}
		~MYGUI_C_DELEGATE () { clear(); }

		bool empty() const { return mDelegate == nullptr; }

		void clear()
		{
			if (mDelegate)
			{
				delete mDelegate;
				mDelegate = nullptr;
			}
		}

		MYGUI_C_DELEGATE  MYGUI_TEMPLATE_ARGS & operator=(IDelegate* _delegate)
		{
			delete mDelegate;
			mDelegate = _delegate;
			return *this;
		}

		MYGUI_C_DELEGATE  MYGUI_TEMPLATE_ARGS & operator=(const MYGUI_C_DELEGATE  MYGUI_TEMPLATE_ARGS& _event)
		{
			// забираем себе владение
			delete mDelegate;
			mDelegate = _event.mDelegate;
			const_cast< MYGUI_C_DELEGATE  MYGUI_TEMPLATE_ARGS& >(_event).mDelegate = nullptr;

			return *this;
		}

		void operator()( MYGUI_PARAMS )
		{
			if (mDelegate == nullptr) return;
			mDelegate->invoke( MYGUI_ARGS );
		}

	private:
		IDelegate * mDelegate;
	};


	// шаблон класса мульти делегата
	MYGUI_TEMPLATE   MYGUI_TEMPLATE_PARAMS
	class MYGUI_C_MULTI_DELEGATE
	{
	public:
		typedef  MYGUI_I_DELEGATE MYGUI_TEMPLATE_ARGS  IDelegate;
		typedef MYGUI_TYPENAME std::list<IDelegate* /*, Allocator<IDelegate*>*/ > ListDelegate;
		typedef MYGUI_TYPENAME ListDelegate::iterator ListDelegateIterator;
		typedef MYGUI_TYPENAME ListDelegate::const_iterator ConstListDelegateIterator;

		MYGUI_C_MULTI_DELEGATE () { }
		~MYGUI_C_MULTI_DELEGATE () { clear(); }

		bool empty() const
		{
		  for (ConstListDelegateIterator iter = mListDelegates.begin(); iter!=mListDelegates.end(); ++iter)
			{
				if (*iter) return false;
			}
			return true;
		}

		void clear()
		{
			for (ListDelegateIterator iter=mListDelegates.begin(); iter!=mListDelegates.end(); ++iter)
			{
				if (*iter)
				{
					delete (*iter);
					(*iter) = nullptr;
				}
			}
		}

		void clear(IDelegateUnlink * _unlink)
		{
			for (ListDelegateIterator iter=mListDelegates.begin(); iter!=mListDelegates.end(); ++iter)
			{
				if ((*iter) && (*iter)->compare(_unlink))
				{
					delete (*iter);
					(*iter) = nullptr;
				}
			}
		}

		MYGUI_C_MULTI_DELEGATE  MYGUI_TEMPLATE_ARGS & operator+=(IDelegate* _delegate)
		{
			for (ListDelegateIterator iter=mListDelegates.begin(); iter!=mListDelegates.end(); ++iter)
			{
				if ((*iter) && (*iter)->compare(_delegate))
				{
					MYGUI_ASSERT(false, "dublicate delegate");
				}
			}
			mListDelegates.push_back(_delegate);
			return *this;
		}

		MYGUI_C_MULTI_DELEGATE  MYGUI_TEMPLATE_ARGS & operator-=(IDelegate* _delegate)
		{
			for (ListDelegateIterator iter=mListDelegates.begin(); iter!=mListDelegates.end(); ++iter)
			{
				if ((*iter) && (*iter)->compare(_delegate))
				{
					// проверяем на идентичность делегатов
					if ((*iter) != _delegate) delete (*iter);
					(*iter) = nullptr;
					break;
				}
			}
			delete _delegate;
			return *this;
		}

		void operator()( MYGUI_PARAMS )
		{
			ListDelegateIterator iter = mListDelegates.begin();
			while (iter != mListDelegates.end())
			{
				if (nullptr == (*iter))
				{
					iter = mListDelegates.erase(iter);
				}
				else
				{
					(*iter)->invoke( MYGUI_ARGS );
					++iter;
				}
			}
		}

	private:
		// constructor and operator =, without implementation, just for private
		MYGUI_C_MULTI_DELEGATE (const MYGUI_C_MULTI_DELEGATE  MYGUI_TEMPLATE_ARGS & _event);
		MYGUI_C_MULTI_DELEGATE  MYGUI_TEMPLATE_ARGS & operator=(const MYGUI_C_MULTI_DELEGATE  MYGUI_TEMPLATE_ARGS & _event);


	private:
		ListDelegate mListDelegates;

	};


	#undef MYGUI_COMBINE
	#undef MYGUI_COMBINE1

	#undef MYGUI_I_DELEGATE

	#undef MYGUI_C_STATIC_DELEGATE
	#undef MYGUI_C_METHOD_DELEGATE

	#undef MYGUI_C_DELEGATE
	#undef MYGUI_C_MULTI_DELEGATE

	#undef MYGUI_SUFFIX
	#undef MYGUI_TEMPLATE
	#undef MYGUI_TEMPLATE_PARAMS
	#undef MYGUI_TEMPLATE_ARGS
	#undef MYGUI_T_TEMPLATE_PARAMS
	#undef MYGUI_T_TEMPLATE_ARGS
	#undef MYGUI_PARAMS
	#undef MYGUI_ARGS
	#undef MYGUI_TYPENAME

} // namespace delegates
