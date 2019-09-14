#ifndef GAME_MWLUA_POINTER_H
#define GAME_MWLUA_POINTER_H

#include <extern/sol2/sol.hpp>

// FIXME: ideally, we should use the osg::ref_ptr for OSG objecs and std::shared_ptr for everethyng else.
// Unfortunately, at the moment sol::make_object does not work properly with osg::ref_ptr, so makeLuaNiPointer() does not work too.

namespace MWLua
{
    template <class T>
    class Pointer
    {
    public:
        Pointer(T * pointer = nullptr)
        {
            claim(pointer);
        }

        Pointer(const Pointer<T>& pointer)
        {
            claim(pointer);
        }

        ~Pointer() {
            release();
        }

        bool operator==(T* pointer) const
        {
            return (pointer == m_Pointer);
        }

        bool operator!=(T* pointer) const
        {
            return (pointer != m_Pointer);
        }

        bool operator==(const Pointer<T>& pointer) const
        {
            return (pointer == m_Pointer);
        }

        bool operator!=(const Pointer<T>& pointer) const
        {
            return (pointer != m_Pointer);
        }

        operator T*() const
        {
            return m_Pointer;
        }

        T* operator->() const
        {
            return m_Pointer;
        }

        Pointer<T>& operator=(const Pointer<T>& pointer)
        {
            if (m_Pointer != pointer)
            {
                claim(pointer);
            }
            return *this;
        }

        Pointer<T>& operator=(T* pointer)
        {
            if (m_Pointer != pointer)
            {
                claim(pointer);
            }
            return *this;
        }

        T * get()
        {
            return m_Pointer;
        }

    private:
        T * m_Pointer = nullptr;

        void release()
        {
            if (m_Pointer)
            {
                m_Pointer->unref();
            }
        }

        void claim(T * pointer)
        {
            release();

            m_Pointer = pointer;
            if (m_Pointer)
            {
                m_Pointer->ref();
            }
        }
    };
}

namespace sol
{
    template <typename T>
    struct unique_usertype_traits<MWLua::Pointer<T>>
    {
        typedef T type;
        typedef MWLua::Pointer<T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type& value)
        {
            return value == nullptr;
        }

        static type* get(const actual_type& p)
        {
            return p;
        }
    };
}

#endif
